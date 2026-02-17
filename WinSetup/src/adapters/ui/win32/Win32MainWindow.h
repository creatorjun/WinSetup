// src/adapters/ui/win32/Win32MainWindow.h
#pragma once

#include <abstractions/ui/IWindow.h>
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <adapters/ui/win32/controls/TypeSelectorGroup.h>
#include <adapters/ui/win32/controls/ToggleButton.h>
#include <adapters/ui/win32/controls/SimpleButton.h>
#include <adapters/ui/win32/Win32ProgressBar.h>
#include <Windows.h>
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class Win32MainWindow : public abstractions::IWindow {
    public:
        Win32MainWindow(
            std::shared_ptr<abstractions::ILogger>        logger,
            std::shared_ptr<abstractions::IMainViewModel> viewModel);

        ~Win32MainWindow() override;

        Win32MainWindow(const Win32MainWindow&) = delete;
        Win32MainWindow& operator=(const Win32MainWindow&) = delete;

        bool Create(HINSTANCE hInstance, int nCmdShow);

        void Show()    override;
        void Hide()    override;
        bool IsValid() const noexcept override;
        bool RunMessageLoop() override;

        [[nodiscard]] HWND GetHWND() const noexcept;

    private:
        void InitializeWidgets();
        void RebuildTypeSelector();
        void DrawStatusText(HDC hdc);

        void OnViewModelPropertyChanged(const std::wstring& propertyName);
        void UpdateStatusText();
        void UpdateTypeDescription();
        void UpdateWindowTitle();
        void UpdateDataPreservation();
        void UpdateBitlockerEnabled();
        void UpdateProcessingState();
        void UpdateProgress();

        void StartTimer();
        void StopTimer();

        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnTimer(WPARAM timerId);

        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        HWND      m_hWnd;
        HINSTANCE m_hInstance;

        std::shared_ptr<abstractions::ILogger>        m_logger;
        std::shared_ptr<abstractions::IMainViewModel> m_viewModel;

        TypeSelectorGroup m_typeSelectorGroup;
        RECT              m_selectorRect;
        std::wstring      m_typeDescription;

        ToggleButton   m_btnDataPreserve;
        ToggleButton   m_btnBitlocker;
        SimpleButton   m_btnStartStop;
        Win32ProgressBar m_progressBar;

        static constexpr int  WINDOW_WIDTH = 640;
        static constexpr int  WINDOW_HEIGHT = 520;
        static constexpr int  TYPE_SELECTOR_GROUP_ID = 100;
        static constexpr int  ID_TOGGLE_DATA_PRESERVE = 4000;
        static constexpr int  ID_TOGGLE_BITLOCKER = 4001;
        static constexpr int  ID_BTN_START_STOP = 4002;
        static constexpr int  ID_PROGRESS_BAR = 4003;
        static constexpr UINT TIMER_ID_PROGRESS = 1001;
        static constexpr auto CLASS_NAME = L"WinSetupMainWindow";
    };

}
