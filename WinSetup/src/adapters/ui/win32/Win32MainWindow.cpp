// src/adapters/ui/win32/Win32MainWindow.cpp
#include "Win32MainWindow.h"
#include <windowsx.h>

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(std::shared_ptr<abstractions::ILogger> logger)
        : m_hwnd(nullptr)
        , m_hInstance(nullptr)
        , m_logger(std::move(logger))
    {
    }

    Win32MainWindow::~Win32MainWindow() {
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
        }
    }

    bool Win32MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
        m_hInstance = hInstance;

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASS_NAME;

        if (!RegisterClassExW(&wc)) {
            if (m_logger) {
                m_logger->Error(L"Failed to register window class");
            }
            return false;
        }

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenWidth - DEFAULT_WIDTH) / 2;
        int posY = (screenHeight - DEFAULT_HEIGHT) / 2;

        m_hwnd = CreateWindowExW(
            0,
            CLASS_NAME,
            L"WinSetup - PC 초기화 프로그램",
            WS_OVERLAPPEDWINDOW,
            posX, posY,
            DEFAULT_WIDTH, DEFAULT_HEIGHT,
            nullptr,
            nullptr,
            hInstance,
            this
        );

        if (!m_hwnd) {
            if (m_logger) {
                m_logger->Error(L"Failed to create window");
            }
            return false;
        }

        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

        if (m_logger) {
            m_logger->Info(L"Main window created successfully");
        }

        return true;
    }

    void Win32MainWindow::Show() {
        if (m_hwnd) {
            ShowWindow(m_hwnd, SW_SHOW);
        }
    }

    void Win32MainWindow::Hide() {
        if (m_hwnd) {
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }

    void Win32MainWindow::SetTitle(const std::wstring& title) {
        if (m_hwnd) {
            SetWindowTextW(m_hwnd, title.c_str());
        }
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Win32MainWindow* pThis = nullptr;

        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = static_cast<Win32MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hwnd = hwnd;
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

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            DestroyWindow(m_hwnd);
            return 0;

        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        }
    }

    void Win32MainWindow::OnCreate() {
        if (m_logger) {
            m_logger->Debug(L"Window WM_CREATE received");
        }
    }

    void Win32MainWindow::OnDestroy() {
        if (m_logger) {
            m_logger->Info(L"Window destroyed");
        }
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        RECT rect;
        GetClientRect(m_hwnd, &rect);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        std::wstring text = L"WinSetup PC 초기화 프로그램\n\n준비 중...";

        DrawTextW(hdc, text.c_str(), -1, &rect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(m_hwnd, &ps);
    }

    void Win32MainWindow::OnSize(UINT width, UINT height) {
        if (m_logger) {
            std::wstring msg = L"Window resized: " +
                std::to_wstring(width) + L"x" + std::to_wstring(height);
            m_logger->Debug(msg);
        }
    }

}
