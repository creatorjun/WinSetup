#include "MainWindow.h"
#include "../../resources/resource.h"
#include <stdexcept>

namespace winsetup::infrastructure {

    MainWindow::MainWindow() = default;

    MainWindow::~MainWindow() {
        if (hIcon_) {
            DestroyIcon(hIcon_);
        }
        if (hIconSmall_) {
            DestroyIcon(hIconSmall_);
        }
        if (hwnd_) {
            DestroyWindow(hwnd_);
        }
    }

    bool MainWindow::Create(
        HINSTANCE hInstance,
        int width,
        int height,
        const std::wstring& title
    ) {
        hInstance_ = hInstance;
        width_ = width;
        height_ = height;

        if (!RegisterWindowClass(hInstance)) {
            return false;
        }

        RECT rect = { 0, 0, width, height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        const int adjustedWidth = rect.right - rect.left;
        const int adjustedHeight = rect.bottom - rect.top;

        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        const int posX = (screenWidth - adjustedWidth) / 2;
        const int posY = (screenHeight - adjustedHeight) / 2;

        hwnd_ = CreateWindowExW(
            0,
            CLASS_NAME,
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            posX,
            posY,
            adjustedWidth,
            adjustedHeight,
            nullptr,
            nullptr,
            hInstance,
            this
        );

        if (!hwnd_) {
            return false;
        }

        [[maybe_unused]] bool iconLoaded = LoadAndSetIcon();

        CreateChildWidgets();

        return true;
    }

    void MainWindow::Show(int nCmdShow) {
        if (hwnd_) {
            ShowWindow(hwnd_, nCmdShow);
            UpdateWindow(hwnd_);
        }
    }

    void MainWindow::Hide() {
        if (hwnd_) {
            ShowWindow(hwnd_, SW_HIDE);
        }
    }

    void MainWindow::Close() {
        if (hwnd_) {
            PostMessage(hwnd_, WM_CLOSE, 0, 0);
        }
    }

    int MainWindow::RunMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

    void MainWindow::SetStatusText(const std::wstring& text) {
        if (statusText_) {
            statusText_->SetText(text);
        }
    }

    std::wstring MainWindow::GetStatusText() const {
        if (statusText_) {
            return statusText_->GetText();
        }
        return L"";
    }

    LRESULT CALLBACK MainWindow::WindowProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
    ) {
        MainWindow* window = nullptr;

        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            window = static_cast<MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            window->OnCreate(hwnd);
        }
        else {
            window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (window) {
            return window->HandleMessage(uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_DESTROY:
            OnDestroy();
            return 0;

        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd_);
            return 0;

        default:
            return DefWindowProc(hwnd_, uMsg, wParam, lParam);
        }
    }

    void MainWindow::OnCreate(HWND hwnd) {
        hwnd_ = hwnd;
    }

    void MainWindow::OnDestroy() {
        hwnd_ = nullptr;
        PostQuitMessage(0);
    }

    void MainWindow::OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);

        RECT clientRect;
        GetClientRect(hwnd_, &clientRect);

        HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdc, &clientRect, hBrush);
        DeleteObject(hBrush);

        EndPaint(hwnd_, &ps);
    }

    void MainWindow::OnSize(UINT width, UINT height) {
        width_ = width;
        height_ = height;
        UpdateChildWidgetPositions();
    }

    bool MainWindow::RegisterWindowClass(HINSTANCE hInstance) {
        static bool registered = false;
        if (registered) {
            return true;
        }

        HICON hIcon = static_cast<HICON>(LoadImageW(
            hInstance,
            MAKEINTRESOURCEW(IDI_MAINICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON),
            LR_DEFAULTCOLOR
        ));

        HICON hIconSmall = static_cast<HICON>(LoadImageW(
            hInstance,
            MAKEINTRESOURCEW(IDI_MAINICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR
        ));

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = hIcon ? hIcon : LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = CLASS_NAME;
        wc.hIconSm = hIconSmall ? hIconSmall : LoadIcon(nullptr, IDI_APPLICATION);

        const ATOM atom = RegisterClassExW(&wc);
        if (atom == 0) {
            if (hIcon) DestroyIcon(hIcon);
            if (hIconSmall) DestroyIcon(hIconSmall);
            return false;
        }

        registered = true;
        return true;
    }

    bool MainWindow::LoadAndSetIcon() {
        if (!hwnd_ || !hInstance_) {
            return false;
        }

        hIcon_ = static_cast<HICON>(LoadImageW(
            hInstance_,
            MAKEINTRESOURCEW(IDI_MAINICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON),
            LR_DEFAULTCOLOR
        ));

        hIconSmall_ = static_cast<HICON>(LoadImageW(
            hInstance_,
            MAKEINTRESOURCEW(IDI_MAINICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR
        ));

        if (hIcon_) {
            SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon_));
        }

        if (hIconSmall_) {
            SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall_));
        }

        return hIcon_ != nullptr || hIconSmall_ != nullptr;
    }

    void MainWindow::CreateChildWidgets() {
        if (!hwnd_ || !hInstance_) {
            return;
        }

        statusText_ = std::make_unique<StatusTextWidget>();
        if (!statusText_->Create(hwnd_, hInstance_, L"시스템 분석중입니다.")) {
            statusText_.reset();
        }

        UpdateChildWidgetPositions();
    }

    void MainWindow::UpdateChildWidgetPositions() {
        if (!statusText_) {
            return;
        }

        const int contentWidth = width_ - (PADDING * 2);
        const int contentHeight = height_ - (PADDING * 2);

        statusText_->UpdatePosition(contentWidth, contentHeight, PADDING, PADDING);
    }

}
