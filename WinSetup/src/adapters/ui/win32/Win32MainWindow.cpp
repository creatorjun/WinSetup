// src/adapters/ui/win32/Win32MainWindow.cpp
#include <adapters/ui/win32/Win32MainWindow.h>
#include <resources/resource.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::ILogger> logger,
        std::shared_ptr<abstractions::IMainViewModel> viewModel
    )
        : mHwnd(nullptr)
        , mHInstance(nullptr)
        , mLogger(std::move(logger))
        , mViewModel(std::move(viewModel))
    {
        if (mViewModel)
            mViewModel->AddPropertyChangedHandler([this](const std::wstring& propertyName) {
            OnViewModelPropertyChanged(propertyName);
                });
    }

    Win32MainWindow::~Win32MainWindow() {
        if (mViewModel)
            mViewModel->RemoveAllPropertyChangedHandlers();
        if (mHwnd)
            DestroyWindow(mHwnd);
    }

    bool Win32MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
        mHInstance = hInstance;

        HICON hIcon = LoadIconW(mHInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        HICON hIconSm = LoadIconW(mHInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!hIcon || !hIconSm) {
            if (mLogger) mLogger->Warning(L"Failed to load application icon, using default");
            hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
        }

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = mHInstance;
        wc.hIcon = hIcon;
        wc.hIconSm = hIconSm;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASSNAME;

        if (!RegisterClassExW(&wc)) {
            if (mLogger) mLogger->Error(L"Failed to register window class");
            return false;
        }

        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT  windowRect{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        AdjustWindowRect(&windowRect, dwStyle, FALSE);

        int adjustedWidth = windowRect.right - windowRect.left;
        int adjustedHeight = windowRect.bottom - windowRect.top;
        int posX = (GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) / 2;
        int posY = (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) / 2;

        std::wstring title = mViewModel ? mViewModel->GetWindowTitle() : L"WinSetup - PC Reinstallation Tool";

        mHwnd = CreateWindowExW(
            0, CLASSNAME, title.c_str(), dwStyle,
            posX, posY, adjustedWidth, adjustedHeight,
            nullptr, nullptr, mHInstance, this
        );

        if (!mHwnd) {
            if (mLogger) mLogger->Error(L"Failed to create window");
            return false;
        }

        ShowWindow(mHwnd, nCmdShow);
        UpdateWindow(mHwnd);

        if (mLogger) mLogger->Info(L"Main window created successfully with MVVM pattern");
        return true;
    }

    void Win32MainWindow::Show() {
        if (mHwnd) ShowWindow(mHwnd, SW_SHOW);
    }

    void Win32MainWindow::Hide() {
        if (mHwnd) ShowWindow(mHwnd, SW_HIDE);
    }

    bool Win32MainWindow::IsValid() const noexcept {
        return mHwnd != nullptr;
    }

    HWND Win32MainWindow::GetHWND() const noexcept {
        return mHwnd;
    }

    bool Win32MainWindow::RunMessageLoop() {
        MSG msg{};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return (msg.wParam == 0);
    }

    void Win32MainWindow::OnViewModelPropertyChanged(const std::wstring& propertyName) {
        if (propertyName == L"StatusText")  UpdateStatusText();
        else if (propertyName == L"WindowTitle") UpdateWindowTitle();
    }

    void Win32MainWindow::UpdateStatusText() {
        if (mHwnd) InvalidateRect(mHwnd, nullptr, TRUE);
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (mHwnd && mViewModel)
            SetWindowTextW(mHwnd, mViewModel->GetWindowTitle().c_str());
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Win32MainWindow* pThis = nullptr;

        if (uMsg == WM_NCCREATE) {
            auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = static_cast<Win32MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->mHwnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<Win32MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (pThis)
            return pThis->HandleMessage(uMsg, wParam, lParam);

        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Win32MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_DESTROY:
            OnDestroy();
            return 0;
        case WM_PAINT:
            OnPaint();
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
            DestroyWindow(mHwnd);
            return 0;
        default:
            return DefWindowProcW(mHwnd, uMsg, wParam, lParam);
        }
    }

    void Win32MainWindow::OnCreate() {
        if (mLogger) mLogger->Debug(L"Window WM_CREATE received");
    }

    void Win32MainWindow::OnDestroy() {
        if (mLogger) mLogger->Info(L"Window destroyed");
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(mHwnd, &ps);
        DrawStatusText(hdc);
        EndPaint(mHwnd, &ps);
    }

    void Win32MainWindow::DrawStatusText(HDC hdc) {
        if (!mViewModel) return;

        RECT clientRect{};
        GetClientRect(mHwnd, &clientRect);

        int  statusHeight = static_cast<int>(clientRect.bottom * STATUS_AREA_HEIGHT_RATIO);
        RECT statusRect = clientRect;
        statusRect.top = 0;
        statusRect.bottom = statusHeight;

        HFONT hFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        std::wstring statusText = mViewModel->GetStatusText();
        DrawTextW(hdc, statusText.c_str(), -1, &statusRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

}
