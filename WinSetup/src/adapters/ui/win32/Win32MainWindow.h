// src/adapters/ui/win32/Win32MainWindow.h
#pragma once

#include <abstractions/ui/IWindow.h>
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/ui/IWidget.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <adapters/ui/win32/controls/TypeSelectorGroup.h>
#include <adapters/ui/win32/panels/StatusPanel.h>
#include <adapters/ui/win32/panels/OptionPanel.h>
#include <adapters/ui/win32/panels/ActionPanel.h>
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

namespace winsetup::adapters::ui {

    class Win32MainWindow : public abstractions::IWindow {
    public:
        Win32MainWindow(
            std::shared_ptr<abstractions::ILogger>        logger,
            std::shared_ptr<abstractions::IMainViewModel> viewModel
        );
        ~Win32MainWindow() override;
        Win32MainWindow(const Win32MainWindow&) = delete;
        Win32MainWindow& operator=(const Win32MainWindow&) = delete;

        bool Create(HINSTANCE hInstance, int nCmdShow);

        void Show() override;
        void Hide() override;
        [[nodiscard]] bool IsValid() const noexcept override;
        [[nodiscard]] bool RunMessageLoop() override;

        [[nodiscard]] HWND GetHWND() const noexcept;

    private:
        void InitializeWidgets();
        void RebuildTypeSelector();
        void OnViewModelPropertyChanged(const std::wstring& propertyName);
        void UpdateWindowTitle();
        void StartTimer();
        void StopTimer();
        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnTimer(WPARAM timerId);

        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        HWND                                          mhWnd;
        HINSTANCE                                     mhInstance;
        std::shared_ptr<abstractions::ILogger>        mlogger;
        std::shared_ptr<abstractions::IMainViewModel> mviewModel;

        TypeSelectorGroup mtypeSelectorGroup;
        RECT              mselectorRect;
        StatusPanel       mstatusPanel;
        OptionPanel       moptionPanel;
        ActionPanel       mactionPanel;

        std::vector<abstractions::IWidget*> mwidgets;

        static constexpr int       WINDOWWIDTH = 640;
        static constexpr int       WINDOWHEIGHT = 430;
        static constexpr int       TYPESELECTORGROUPID = 100;
        static constexpr UINT_PTR  MAIN_TIMER_ID = 2001;
        static constexpr auto      CLASSNAME = L"WinSetupMainWindow";
    };

}
