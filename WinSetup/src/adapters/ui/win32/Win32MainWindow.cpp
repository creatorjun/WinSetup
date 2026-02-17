// src/adapters/ui/win32/Win32MainWindow.cpp
#include "Win32MainWindow.h"
#include <resources/resource.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::ILogger> logger,
        std::shared_ptr<abstractions::IMainViewModel> viewModel
    )
        : mhwnd(nullptr)
        , mhInstance(nullptr)
        , mLogger(std::move(logger))
        , mViewModel(std::move(viewModel))
    {
        if (mViewModel) {
            mViewModel->AddPropertyChangedHandler([this](const std::wstring& propertyName) {
                OnViewModelPropertyChanged(propertyName);
                });
        }
    }

    Win32MainWindow::~Win32MainWindow() {
        if (mViewModel) {
            mViewModel->RemoveAllPropertyChangedHandlers();
        }
        if (mhwnd) {
            DestroyWindow(mhwnd);
        }
    }

    bool Win32MainWindow::Create(void* hInstance, int nCmdShow) {
        mhInstance = static_cast<HINSTANCE>(hInstance);

        HICON hIcon = LoadIconW(mhInstance, MAKEINTRESOURCE(IDI_MAINICON));
        HICON hIconSm = LoadIconW(mhInstance, MAKEINTRESOURCE(IDI_MAINICON));

        if (!hIcon || !hIconSm) {
            if (mLogger) {
                mLogger->Warning(L"Failed to load application icon, using default");
            }
            hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
        }

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = mhInstance;
        wc.hIcon = hIcon;
        wc.hIconSm = hIconSm;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASSNAME;

        if (!RegisterClassExW(&wc)) {
            if (mLogger) {
                mLogger->Error(L"Failed to register window class");
            }
            return false;
        }

        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        RECT windowRect{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        AdjustWindowRect(&windowRect, dwStyle, FALSE);

        int adjustedWidth = windowRect.right - windowRect.left;
        int adjustedHeight = windowRect.bottom - windowRect.top;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenWidth - adjustedWidth) / 2;
        int posY = (screenHeight - adjustedHeight) / 2;

        std::wstring title = mViewModel ? mViewModel->GetWindowTitle() : L"WinSetup - PC";

        mhwnd = CreateWindowExW(
            0,
            CLASSNAME,
            title.c_str(),
            dwStyle,
            posX, posY,
            adjustedWidth, adjustedHeight,
            nullptr,
            nullptr,
            mhInstance,
            this
        );

        if (!mhwnd) {
            if (mLogger) {
                mLogger->Error(L"Failed to create window");
            }
            return false;
        }

        ShowWindow(mhwnd, nCmdShow);
        UpdateWindow(mhwnd);

        if (mLogger) {
            mLogger->Info(L"Main window created successfully with MVVM pattern");
        }

        return true;
    }

    void Win32MainWindow::Show() {
        if (mhwnd) {
            ShowWindow(mhwnd, SW_SHOW);
        }
    }

    void Win32MainWindow::Hide() {
        if (mhwnd) {
            ShowWindow(mhwnd, SW_HIDE);
        }
    }

    void Win32MainWindow::OnViewModelPropertyChanged(const std::wstring& propertyName) {
        if (propertyName == L"StatusText") {
            UpdateStatusText();
        }
        else if (propertyName == L"WindowTitle") {
            UpdateWindowTitle();
        }
    }

    void Win32MainWindow::UpdateStatusText() {
        if (mhwnd) {
            InvalidateRect(mhwnd, nullptr, TRUE);
        }
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (mhwnd && mViewModel) {
            SetWindowTextW(mhwnd, mViewModel->GetWindowTitle().c_str());
        }
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Win32MainWindow* pThis = nullptr;

        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = static_cast<Win32MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->mhwnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<Win32MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (pThis) {
            return pThis->HandleMessage(uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
            lpMMI->ptMinTrackSize.x = WINDOW_WIDTH;
            lpMMI->ptMinTrackSize.y = WINDOW_HEIGHT;
            lpMMI->ptMaxTrackSize.x = WINDOW_WIDTH;
            lpMMI->ptMaxTrackSize.y = WINDOW_HEIGHT;
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(mhwnd);
            return 0;

        default:
            return DefWindowProc(mhwnd, uMsg, wParam, lParam);
        }
    }

    void Win32MainWindow::OnCreate() {
        if (mLogger) {
            mLogger->Debug(L"Window WM_CREATE received");
        }
    }

    void Win32MainWindow::OnDestroy() {
        if (mLogger) {
            mLogger->Info(L"Window destroyed");
        }
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(mhwnd, &ps);
        DrawStatusText(hdc);
        EndPaint(mhwnd, &ps);
    }

    void Win32MainWindow::DrawStatusText(HDC hdc) {
        if (!mViewModel) {
            return;
        }

        RECT clientRect;
        GetClientRect(mhwnd, &clientRect);

        int statusHeight = static_cast<int>(clientRect.bottom * STATUSAREA_HEIGHT_RATIO);
        RECT statusRect = clientRect;
        statusRect.top = 0;
        statusRect.bottom = statusHeight;

        HFONT hFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );

        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        std::wstring statusText = mViewModel->GetStatusText();
        DrawTextW(
            hdc,
            statusText.c_str(),
            -1,
            &statusRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }


}
