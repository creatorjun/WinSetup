// src/adapters/ui/win32/Win32MainWindow.h
#pragma once

#include <Windows.h>
#include <string>
#include <memory>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/ui/IWindow.h>

namespace winsetup::adapters::ui {

    class Win32MainWindow : public abstractions::IWindow {
    public:
        explicit Win32MainWindow(
            std::shared_ptr<abstractions::ILogger> logger,
            std::shared_ptr<abstractions::IMainViewModel> viewModel
        );
        ~Win32MainWindow() override;

        Win32MainWindow(const Win32MainWindow&) = delete;
        Win32MainWindow& operator=(const Win32MainWindow&) = delete;

        bool Create(void* hInstance, int nCmdShow) override;
        void Show() override;
        void Hide() override;
        void* GetHandle() const noexcept override { return mhwnd; }

        [[nodiscard]] HWND GetHWND() const noexcept { return mhwnd; }

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void DrawStatusText(HDC hdc);

        void OnViewModelPropertyChanged(const std::wstring& propertyName);
        void UpdateStatusText();
        void UpdateWindowTitle();

        HWND mhwnd;
        HINSTANCE mhInstance;
        std::shared_ptr<abstractions::ILogger> mLogger;
        std::shared_ptr<abstractions::IMainViewModel> mViewModel;

        static constexpr const wchar_t* CLASSNAME = L"WinSetupMainWindow";
        static constexpr int WINDOW_WIDTH = 640;
        static constexpr int WINDOW_HEIGHT = 480;
        static constexpr float STATUSAREA_HEIGHT_RATIO = 0.15f;
    };

}
