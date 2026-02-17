// src/adapters/ui/win32/Win32MainWindow.cpp
#include <adapters/ui/win32/Win32MainWindow.h>
#include <resources/resource.h>
#include <windowsx.h>
#include <vector>

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::ILogger> logger,
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
        : m_hwnd(nullptr)
        , m_hInstance(nullptr)
        , m_logger(std::move(logger))
        , m_viewModel(std::move(viewModel))
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
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    bool Win32MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
        m_hInstance = hInstance;

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

        m_hwnd = CreateWindowExW(
            0, CLASS_NAME, title.c_str(), dwStyle,
            posX, posY, adjustedWidth, adjustedHeight,
            nullptr, nullptr, m_hInstance, this);

        if (!m_hwnd) {
            if (m_logger) m_logger->Error(L"Failed to create window");
            return false;
        }

        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

        if (m_logger) m_logger->Info(L"Main window created successfully");
        return true;
    }

    void Win32MainWindow::Show() { if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW); }
    void Win32MainWindow::Hide() { if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE); }
    bool Win32MainWindow::IsValid() const noexcept { return m_hwnd != nullptr; }
    HWND Win32MainWindow::GetHWND() const noexcept { return m_hwnd; }

    bool Win32MainWindow::RunMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return msg.wParam == 0;
    }

    void Win32MainWindow::InitializeWidgets() {
        TextWidgetStyle statusStyle;
        statusStyle.fontSize = 18;
        statusStyle.fontWeight = FW_NORMAL;
        statusStyle.textColor = RGB(0, 0, 0);
        statusStyle.placeholderColor = RGB(160, 160, 160);
        statusStyle.bgColor = RGB(255, 255, 255);
        statusStyle.drawBackground = false;
        statusStyle.drawTopBorder = false;
        statusStyle.drawBottomBorder = false;
        statusStyle.dtFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
        statusStyle.paddingLeft = PAD_TEXT;
        statusStyle.paddingRight = PAD_TEXT;
        statusStyle.paddingTop = 0;
        statusStyle.paddingBottom = 0;
        m_statusWidget.SetStyle(statusStyle);

        if (m_viewModel)
            m_statusWidget.SetText(m_viewModel->GetStatusText());

        TextWidgetStyle typeStyle;
        typeStyle.fontSize = 14;
        typeStyle.fontWeight = FW_NORMAL;
        typeStyle.textColor = RGB(30, 30, 30);
        typeStyle.placeholderColor = RGB(160, 160, 160);
        typeStyle.bgColor = RGB(245, 245, 245);
        typeStyle.drawBackground = true;
        typeStyle.drawTopBorder = true;
        typeStyle.drawBottomBorder = true;
        typeStyle.borderColor = RGB(210, 210, 210);
        typeStyle.dtFormat = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
        typeStyle.paddingLeft = MARGIN_H + PAD_TEXT;
        typeStyle.paddingRight = MARGIN_H + PAD_TEXT;
        typeStyle.paddingTop = 0;
        typeStyle.paddingBottom = 0;
        m_typeWidget.SetStyle(typeStyle);
        m_typeWidget.SetPlaceholder(L"타입을 선택해주세요.");

        if (m_viewModel)
            m_typeWidget.SetText(m_viewModel->GetTypeDescription());

        RecalcWidgetRects();
    }

    void Win32MainWindow::RecalcWidgetRects() {
        if (!m_hwnd) return;

        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);

        const int cw = clientRect.right;
        const int ch = clientRect.bottom;
        const int statusHeight = static_cast<int>(ch * STATUS_AREA_HEIGHT_RATIO);
        const int typeHeight = static_cast<int>(ch * TYPE_AREA_HEIGHT_RATIO);

        RECT statusRect = {
            MARGIN_H,
            MARGIN_V,
            cw - MARGIN_H,
            statusHeight - MARGIN_V
        };

        RECT typeRect = {
            0,
            statusHeight,
            cw,
            statusHeight + typeHeight
        };

        m_statusWidget.SetRect(statusRect);
        m_typeWidget.SetRect(typeRect);
    }

    void Win32MainWindow::OnViewModelPropertyChanged(const std::wstring& propertyName) {
        if (propertyName == L"StatusText")      UpdateStatusText();
        else if (propertyName == L"TypeDescription") UpdateTypeDescription();
        else if (propertyName == L"WindowTitle")     UpdateWindowTitle();
    }

    void Win32MainWindow::UpdateStatusText() {
        if (!m_viewModel) return;
        m_statusWidget.SetText(m_viewModel->GetStatusText());
        m_statusWidget.Invalidate(m_hwnd);
    }

    void Win32MainWindow::UpdateTypeDescription() {
        if (!m_viewModel) return;
        std::wstring desc = m_viewModel->GetTypeDescription();
        m_typeWidget.SetText(desc == L"타입을 선택해주세요." ? L"" : desc);
        m_typeWidget.Invalidate(m_hwnd);
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (m_hwnd && m_viewModel)
            SetWindowTextW(m_hwnd, m_viewModel->GetWindowTitle().c_str());
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Win32MainWindow* pThis = nullptr;
        if (uMsg == WM_NCCREATE) {
            auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = static_cast<Win32MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hwnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<Win32MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Win32MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CREATE:   OnCreate();                 return 0;
        case WM_DESTROY:  OnDestroy();                return 0;
        case WM_PAINT:    OnPaint();                  return 0;
        case WM_COMMAND:  OnCommand(wParam, lParam);  return 0;
        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_GETMINMAXINFO: {
            auto* lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
            lpMMI->ptMinTrackSize.x = WINDOW_WIDTH;
            lpMMI->ptMinTrackSize.y = WINDOW_HEIGHT;
            lpMMI->ptMaxTrackSize.x = WINDOW_WIDTH;
            lpMMI->ptMaxTrackSize.y = WINDOW_HEIGHT;
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(m_hwnd);
            return 0;
        default:
            return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
        }
    }

    void Win32MainWindow::OnCreate() {
        if (m_logger) m_logger->Debug(L"Window WM_CREATE received");
        InitializeWidgets();
    }

    void Win32MainWindow::OnDestroy() {
        if (m_logger) m_logger->Info(L"Window destroyed");
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        m_statusWidget.Draw(hdc);
        m_typeWidget.Draw(hdc);

        EndPaint(m_hwnd, &ps);
    }

    void Win32MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
        if (HIWORD(wParam) != BN_CLICKED) return;
        if (!m_viewModel) return;

        HWND hCtrl = reinterpret_cast<HWND>(lParam);
        if (!hCtrl) return;

        int len = GetWindowTextLengthW(hCtrl);
        if (len <= 0) return;

        std::vector<wchar_t> buf(static_cast<size_t>(len) + 1);
        GetWindowTextW(hCtrl, buf.data(), len + 1);

        m_viewModel->SetTypeDescription(std::wstring(buf.data()));
    }

    void Win32MainWindow::OnSize(int clientWidth, int clientHeight) {
        (void)clientWidth;
        (void)clientHeight;
        RecalcWidgetRects();
    }

}
