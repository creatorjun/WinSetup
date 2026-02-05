#pragma once

#include <Windows.h>
#include <string>
#include <memory>

namespace winsetup::infrastructure {

    class MainWindow {
    public:
        MainWindow();
        ~MainWindow();

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator=(const MainWindow&) = delete;
        MainWindow(MainWindow&&) = delete;
        MainWindow& operator=(MainWindow&&) = delete;

        [[nodiscard]] bool Create(
            HINSTANCE hInstance,
            int width,
            int height,
            const std::wstring& title
        );

        void Show(int nCmdShow);
        void Hide();
        void Close();

        [[nodiscard]] HWND GetHandle() const noexcept { return hwnd_; }
        [[nodiscard]] bool IsCreated() const noexcept { return hwnd_ != nullptr; }

        [[nodiscard]] int RunMessageLoop();

    private:
        static LRESULT CALLBACK WindowProc(
            HWND hwnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam
        );

        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

        void OnCreate(HWND hwnd);
        void OnDestroy();
        void OnPaint();
        void OnSize(UINT width, UINT height);

        [[nodiscard]] static bool RegisterWindowClass(HINSTANCE hInstance);
        [[nodiscard]] bool LoadAndSetIcon();

        HWND hwnd_{ nullptr };
        HINSTANCE hInstance_{ nullptr };
        HICON hIcon_{ nullptr };
        HICON hIconSmall_{ nullptr };
        int width_{ 640 };
        int height_{ 480 };

        static constexpr const wchar_t* CLASS_NAME = L"WinSetupMainWindow";
    };

}
