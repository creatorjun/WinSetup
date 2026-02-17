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

        void OnViewModelPropertyChanged(const std::wstring& propertyName);
        void UpdateWindowTitle();

        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnTimer(WPARAM timerId);

        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
            WPARAM wParam, LPARAM lParam);

        HWND      m_hWnd;
        HINSTANCE m_hInstance;

        std::shared_ptr<abstractions::ILogger>        m_logger;
        std::shared_ptr<abstractions::IMainViewModel> m_viewModel;

        TypeSelectorGroup m_typeSelectorGroup;
        RECT              m_selectorRect;

        StatusPanel m_statusPanel;
        OptionPanel m_optionPanel;
        ActionPanel m_actionPanel;

        std::vector<abstractions::IWidget*> m_widgets;

        static constexpr int  WINDOW_WIDTH = 640;
        static constexpr int  WINDOW_HEIGHT = 520;
        static constexpr int  TYPE_SELECTOR_GROUP_ID = 100;
        static constexpr auto CLASS_NAME = L"WinSetupMainWindow";
    };

}
