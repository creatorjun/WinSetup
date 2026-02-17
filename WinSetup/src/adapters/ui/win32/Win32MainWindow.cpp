// src/adapters/ui/win32/Win32MainWindow.cpp

#include <adapters/ui/win32/Win32MainWindow.h>
#include <adapters/ui/win32/controls/ToggleButton.h>
#include <resources/resource.h>
#include <windowsx.h>

#undef min
#undef max

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::ILogger>        logger,
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
        : m_hWnd(nullptr)
        , m_hInstance(nullptr)
        , m_logger(std::move(logger))
        , m_viewModel(std::move(viewModel))
        , m_selectorRect{}
    {
        if (m_viewModel)
            m_viewModel->AddPropertyChangedHandler(
                [this](const std::wstring& propertyName) {
                    OnViewModelPropertyChanged(propertyName);
                });
    }

    Win32MainWindow::~Win32MainWindow() {
        if (m_viewModel)
            m_viewModel->RemoveAllPropertyChangedHandlers();
        if (m_hWnd) {
            DestroyWindow(m_hWnd);
            m_hWnd = nullptr;
        }
    }

    bool Win32MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
        m_hInstance = hInstance;

        ToggleButton::Initialize(m_hInstance);

        HICON hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        HICON hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!hIcon || !hIconSm) {
            if (m_logger) m_logger->Warning(L"Failed to load application icon, using default");
            hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
        }

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = m_hInstance;
        wc.hIcon = hIcon;
        wc.hIconSm = hIconSm;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASS_NAME;

        if (!RegisterClassExW(&wc)) {
            if (m_logger) m_logger->Error(L"Failed to register window class");
            return false;
        }

        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT  windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        AdjustWindowRect(&windowRect, dwStyle, FALSE);

        const int adjustedWidth = windowRect.right - windowRect.left;
        const int adjustedHeight = windowRect.bottom - windowRect.top;
        const int posX = (GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) / 2;
        const int posY = (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) / 2;

        const std::wstring title = m_viewModel
            ? m_viewModel->GetWindowTitle()
            : L"WinSetup - PC Reinstallation Tool";

        m_hWnd = CreateWindowExW(
            0, CLASS_NAME, title.c_str(), dwStyle,
            posX, posY, adjustedWidth, adjustedHeight,
            nullptr, nullptr, m_hInstance, this);

        if (!m_hWnd) {
            if (m_logger) m_logger->Error(L"Failed to create window");
            return false;
        }

        ShowWindow(m_hWnd, nCmdShow);
        UpdateWindow(m_hWnd);

        if (m_logger) m_logger->Info(L"Main window created successfully");
        return true;
    }

    void Win32MainWindow::Show() { if (m_hWnd) ShowWindow(m_hWnd, SW_SHOW); }
    void Win32MainWindow::Hide() { if (m_hWnd) ShowWindow(m_hWnd, SW_HIDE); }
    bool Win32MainWindow::IsValid() const noexcept { return m_hWnd != nullptr; }
    HWND Win32MainWindow::GetHWND() const noexcept { return m_hWnd; }

    bool Win32MainWindow::RunMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return msg.wParam == 0;
    }

    void Win32MainWindow::InitializeWidgets() {
        RECT clientRect = {};
        GetClientRect(m_hWnd, &clientRect);

        const int cw = clientRect.right;
        const int marginH = 16;
        const int statusH = 60;
        const int typeDescH = 40;
        const int gap = 8;
        const int innerPadTop = 28;
        const int innerPadBot = 12;
        const int btnRows = 2;
        const int btnH = 32;
        const int btnGapV = 8;
        const int selectorH = innerPadTop + (btnRows * btnH)
            + ((btnRows - 1) * btnGapV) + innerPadBot;
        const int panelW = cw - marginH * 2;

        // ── StatusPanel ────────────────────────────────────────────────
        const int statusPanelH = statusH + typeDescH + gap;
        m_statusPanel.SetViewModel(m_viewModel);
        m_statusPanel.Create(m_hWnd, m_hInstance,
            marginH, 0, panelW, statusPanelH);

        // ── TypeSelectorGroup ──────────────────────────────────────────
        const int selectorY = statusPanelH + gap;
        m_selectorRect = {
            marginH,
            selectorY,
            cw - marginH,
            selectorY + selectorH
        };

        m_typeSelectorGroup.Create(
            m_hWnd, m_hInstance, L"설치 유형", TYPE_SELECTOR_GROUP_ID);
        m_typeSelectorGroup.SetRect(m_selectorRect);
        m_typeSelectorGroup.SetSelectionChangedCallback(
            [this](const std::wstring& key) {
                if (m_viewModel) m_viewModel->SetTypeDescription(key);
            });

        RebuildTypeSelector();

        // ── OptionPanel ────────────────────────────────────────────────
        const int optionY = m_selectorRect.bottom + gap * 2;
        const int optionH = 36 * 2 + gap;
        m_optionPanel.SetViewModel(m_viewModel);
        m_optionPanel.Create(m_hWnd, m_hInstance,
            marginH, optionY, panelW, optionH);

        // ── ActionPanel ────────────────────────────────────────────────
        const int actionY = optionY + optionH + gap;
        const int actionH = 44 + gap * 2 + 36;
        m_actionPanel.SetViewModel(m_viewModel);
        m_actionPanel.Create(m_hWnd, m_hInstance,
            marginH, actionY, panelW, actionH);

        // ── 위젯 목록 등록 (순회용) ────────────────────────────────────
        m_widgets.clear();
        m_widgets.push_back(&m_statusPanel);
        m_widgets.push_back(&m_optionPanel);
        m_widgets.push_back(&m_actionPanel);
    }

    void Win32MainWindow::RebuildTypeSelector() {
        if (!m_viewModel || !m_hWnd) return;
        const auto types = m_viewModel->GetInstallationTypes();
        if (types.empty()) return;
        m_typeSelectorGroup.Rebuild(types);
    }

    void Win32MainWindow::OnViewModelPropertyChanged(
        const std::wstring& propertyName)
    {
        if (propertyName == L"WindowTitle") {
            UpdateWindowTitle();
            return;
        }
        if (propertyName == L"InstallationTypes") {
            RebuildTypeSelector();
            return;
        }
        if (propertyName == L"IsProcessing" && m_viewModel) {
            const bool processing = m_viewModel->IsProcessing();
            m_typeSelectorGroup.SetEnabled(!processing);
        }

        for (auto* widget : m_widgets)
            widget->OnPropertyChanged(propertyName);
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (m_hWnd && m_viewModel)
            SetWindowTextW(m_hWnd, m_viewModel->GetWindowTitle().c_str());
    }

    void Win32MainWindow::OnCreate() {
        if (m_logger) m_logger->Debug(L"Window WM_CREATE received");
        InitializeWidgets();
    }

    void Win32MainWindow::OnDestroy() {
        if (m_logger) m_logger->Info(L"Window destroyed");
        ToggleButton::Cleanup();
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hWnd, &ps);

        for (auto* widget : m_widgets)
            widget->OnPaint(hdc);

        m_typeSelectorGroup.OnPaint(hdc);

        EndPaint(m_hWnd, &ps);
    }

    void Win32MainWindow::OnTimer(WPARAM timerId) {
        for (auto* widget : m_widgets)
            widget->OnTimer(static_cast<UINT_PTR>(timerId));
    }

    void Win32MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
        for (auto* widget : m_widgets) {
            if (widget->OnCommand(wParam, lParam))
                return;
        }
        m_typeSelectorGroup.OnCommand(wParam, lParam);
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(
        HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Win32MainWindow* pThis = nullptr;
        if (uMsg == WM_NCCREATE) {
            auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = static_cast<Win32MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hWnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<Win32MainWindow*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Win32MainWindow::HandleMessage(
        UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
        case WM_CREATE:  OnCreate();                return 0;
        case WM_DESTROY: OnDestroy();               return 0;
        case WM_PAINT:   OnPaint();                 return 0;
        case WM_COMMAND: OnCommand(wParam, lParam); return 0;
        case WM_TIMER:   OnTimer(wParam);           return 0;
        case WM_CLOSE:
            DestroyWindow(m_hWnd);
            return 0;
        default:
            return DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
        }
    }

}
