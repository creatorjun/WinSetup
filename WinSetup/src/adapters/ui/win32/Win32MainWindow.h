// src/adapters/ui/win32/Win32MainWindow.h
#pragma once

#include <abstractions/ui/IWindow.h>
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <adapters/ui/win32/controls/TextWidget.h>
#include <Windows.h>
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class Win32MainWindow : public abstractions::IWindow {
    public:
        explicit Win32MainWindow(
            std::shared_ptr<abstractions::ILogger> logger,
            std::shared_ptr<abstractions::IMainViewModel> viewModel);
        ~Win32MainWindow() override;

        Win32MainWindow(const Win32MainWindow&) = delete;
        Win32MainWindow& operator=(const Win32MainWindow&) = delete;

        bool Create(HINSTANCE hInstance, int nCmdShow);

        void Show()                                 override;
        void Hide()                                 override;
        [[nodiscard]] bool IsValid() const noexcept override;
        [[nodiscard]] bool RunMessageLoop()         override;
        [[nodiscard]] HWND GetHWND() const noexcept;

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnSize(int clientWidth, int clientHeight);

        void InitializeWidgets();
        void RecalcWidgetRects();

        void OnViewModelPropertyChanged(const std::wstring& propertyName);
        void UpdateStatusText();
        void UpdateTypeDescription();
        void UpdateWindowTitle();

        HWND      m_hwnd;
        HINSTANCE m_hInstance;

        std::shared_ptr<abstractions::ILogger>        m_logger;
        std::shared_ptr<abstractions::IMainViewModel> m_viewModel;

        TextWidget m_statusWidget;
        TextWidget m_typeWidget;

        static constexpr const wchar_t* CLASS_NAME = L"WinSetupMainWindow";
        static constexpr int            WINDOW_WIDTH = 640;
        static constexpr int            WINDOW_HEIGHT = 480;
        static constexpr float          STATUS_AREA_HEIGHT_RATIO = 0.15f;
        static constexpr float          TYPE_AREA_HEIGHT_RATIO = 0.15f;

        static constexpr int MARGIN_H = 16;
        static constexpr int MARGIN_V = 0;
        static constexpr int PAD_TEXT = 12;
    };

}
