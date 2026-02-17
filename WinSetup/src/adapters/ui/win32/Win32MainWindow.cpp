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

        int adjustedWidth = windowRect.right - windowRect.left;
        int adjustedHeight = windowRect.bottom - windowRect.top;
        int posX = (GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) / 2;
        int posY = (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) / 2;

        std::wstring title = m_viewModel
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
        const int btnRows = 2;
        const int btnH = 32;
        const int btnGapV = 8;
        const int innerPadTop = 28;
        const int innerPadBot = 12;
        const int selectorH = innerPadTop + (btnRows * btnH) + ((btnRows - 1) * btnGapV) + innerPadBot;
        const int selectorY = statusH + typeDescH + gap;

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

        // ── 옵션 토글 버튼 (1열 2행) ────────────────────────────────────
        const int optionAreaY = m_selectorRect.bottom + gap * 2;
        const int optionBtnW = cw - marginH * 2;
        const int optionBtnH = 36;

        m_btnDataPreserve.Create(
            m_hWnd, L"데이터 보존",
            marginH, optionAreaY,
            optionBtnW, optionBtnH,
            ID_TOGGLE_DATA_PRESERVE, m_hInstance);

        m_btnBitlocker.Create(
            m_hWnd, L"BitLocker 설정",
            marginH, optionAreaY + optionBtnH + gap,
            optionBtnW, optionBtnH,
            ID_TOGGLE_BITLOCKER, m_hInstance);

        if (m_viewModel) {
            m_btnDataPreserve.SetChecked(m_viewModel->GetDataPreservation());
            m_btnBitlocker.SetChecked(m_viewModel->GetBitlockerEnabled());
        }

        // ── 시작/중지 버튼 ────────────────────────────────────────────────
        const int startStopY = optionAreaY + (optionBtnH + gap) * 2 + gap;
        const int startStopH = 44;

        m_btnStartStop.Create(
            m_hWnd, L"시작",
            marginH, startStopY,
            optionBtnW, startStopH,
            ID_BTN_START_STOP, m_hInstance);

        m_btnStartStop.SetFontSize(15);
    }

    void Win32MainWindow::RebuildTypeSelector() {
        if (!m_viewModel || !m_hWnd) return;
        const auto types = m_viewModel->GetInstallationTypes();
        if (types.empty()) return;
        m_typeSelectorGroup.Rebuild(types);
    }

    void Win32MainWindow::DrawStatusText(HDC hdc) {
        if (!m_viewModel) return;

        RECT clientRect = {};
        GetClientRect(m_hWnd, &clientRect);

        const int marginH = 16;
        const int statusH = 60;
        const int typeDescH = 40;
        const int gap = 8;

        RECT statusRect = { 0, 0, clientRect.right, statusH };

        HFONT hFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextW(hdc, m_viewModel->GetStatusText().c_str(), -1, &statusRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        RECT typeRect = {
            marginH, statusH + gap,
            clientRect.right - marginH, statusH + gap + typeDescH
        };

        HBRUSH hBg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &typeRect, hBg);
        DeleteObject(hBg);

        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hBorderPen));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, typeRect.left, typeRect.top, typeRect.right, typeRect.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hBorderPen);

        HFONT hTypeFont = CreateFontW(
            14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldTypeFont = static_cast<HFONT>(SelectObject(hdc, hTypeFont));

        const bool         empty = m_typeDescription.empty();
        const std::wstring text = empty ? L"타입을 선택해주세요." : m_typeDescription;

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, empty ? RGB(160, 160, 160) : RGB(30, 30, 30));
        DrawTextW(hdc, text.c_str(), -1, &typeRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldTypeFont);
        DeleteObject(hTypeFont);
    }

    void Win32MainWindow::OnViewModelPropertyChanged(const std::wstring& propertyName) {
        if (propertyName == L"StatusText")        UpdateStatusText();
        else if (propertyName == L"TypeDescription")   UpdateTypeDescription();
        else if (propertyName == L"WindowTitle")       UpdateWindowTitle();
        else if (propertyName == L"InstallationTypes") RebuildTypeSelector();
        else if (propertyName == L"DataPreservation")  UpdateDataPreservation();
        else if (propertyName == L"BitlockerEnabled")  UpdateBitlockerEnabled();
        else if (propertyName == L"IsProcessing")      UpdateProcessingState();
    }

    void Win32MainWindow::UpdateStatusText() {
        if (m_hWnd) InvalidateRect(m_hWnd, nullptr, TRUE);
    }

    void Win32MainWindow::UpdateTypeDescription() {
        if (!m_viewModel || !m_hWnd) return;
        m_typeDescription = m_viewModel->GetTypeDescription();
        InvalidateRect(m_hWnd, nullptr, TRUE);
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (m_hWnd && m_viewModel)
            SetWindowTextW(m_hWnd, m_viewModel->GetWindowTitle().c_str());
    }

    void Win32MainWindow::UpdateDataPreservation() {
        if (!m_viewModel || !m_btnDataPreserve.Handle()) return;
        m_btnDataPreserve.SetChecked(m_viewModel->GetDataPreservation());
        InvalidateRect(m_btnDataPreserve.Handle(), nullptr, TRUE);
    }

    void Win32MainWindow::UpdateBitlockerEnabled() {
        if (!m_viewModel || !m_btnBitlocker.Handle()) return;
        m_btnBitlocker.SetChecked(m_viewModel->GetBitlockerEnabled());
        InvalidateRect(m_btnBitlocker.Handle(), nullptr, TRUE);
    }

    void Win32MainWindow::UpdateProcessingState() {
        if (!m_viewModel || !m_btnStartStop.Handle()) return;

        const bool processing = m_viewModel->IsProcessing();

        m_btnStartStop.SetText(processing ? L"중지" : L"시작");

        m_btnDataPreserve.SetEnabled(!processing);
        m_btnBitlocker.SetEnabled(!processing);
        m_typeSelectorGroup.SetEnabled(!processing);
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
        DrawStatusText(hdc);
        m_typeSelectorGroup.OnPaint(hdc);
        EndPaint(m_hWnd, &ps);
    }

    void Win32MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
        const int ctrlId = LOWORD(wParam);
        const int notifCode = HIWORD(wParam);

        if (notifCode == BN_CLICKED) {
            if (ctrlId == ID_TOGGLE_DATA_PRESERVE && m_viewModel) {
                const bool current = m_btnDataPreserve.IsChecked();
                m_viewModel->SetDataPreservation(current);
                if (m_logger)
                    m_logger->Debug(
                        std::wstring(L"DataPreservation toggled: ") +
                        (current ? L"ON" : L"OFF"));
                return;
            }
            if (ctrlId == ID_TOGGLE_BITLOCKER && m_viewModel) {
                const bool current = m_btnBitlocker.IsChecked();
                m_viewModel->SetBitlockerEnabled(current);
                if (m_logger)
                    m_logger->Debug(
                        std::wstring(L"BitlockerEnabled toggled: ") +
                        (current ? L"ON" : L"OFF"));
                return;
            }
            if (ctrlId == ID_BTN_START_STOP && m_viewModel) {
                const bool nowProcessing = !m_viewModel->IsProcessing();
                m_viewModel->SetProcessing(nowProcessing);
                if (m_logger)
                    m_logger->Info(
                        std::wstring(L"Process ") +
                        (nowProcessing ? L"started" : L"stopped"));
                return;
            }
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
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hWnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<Win32MainWindow*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Win32MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CREATE:   OnCreate();                return 0;
        case WM_DESTROY:  OnDestroy();               return 0;
        case WM_PAINT:    OnPaint();                 return 0;
        case WM_COMMAND:  OnCommand(wParam, lParam); return 0;
        case WM_CLOSE:
            DestroyWindow(m_hWnd);
            return 0;
        default:
            return DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
        }
    }

}
