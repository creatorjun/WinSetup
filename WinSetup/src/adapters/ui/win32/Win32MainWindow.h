// src/adapters/ui/win32/Win32MainWindow.h
#pragma once

#include <Windows.h>
#include <string>
#include <memory>
#include <abstractions/infrastructure/logging/ILogger.h>

namespace winsetup::adapters::ui {

    class Win32MainWindow {
    public:
        explicit Win32MainWindow(std::shared_ptr<abstractions::ILogger> logger);
        ~Win32MainWindow();

        Win32MainWindow(const Win32MainWindow&) = delete;
        Win32MainWindow& operator=(const Win32MainWindow&) = delete;

        [[nodiscard]] bool Create(HINSTANCE hInstance, int nCmdShow);
        [[nodiscard]] HWND GetHandle() const noexcept { return m_hwnd; }

        void Show();
        void Hide();
        void SetTitle(const std::wstring& title);

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void OnSize(UINT width, UINT height);

        HWND m_hwnd;
        HINSTANCE m_hInstance;
        std::shared_ptr<abstractions::ILogger> m_logger;

        static constexpr const wchar_t* CLASS_NAME = L"WinSetupMainWindow";
        static constexpr int DEFAULT_WIDTH = 800;
        static constexpr int DEFAULT_HEIGHT = 600;
    };

}
