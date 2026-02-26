#pragma once
#include "abstractions/ui/IWindow.h"
#include "abstractions/ui/IMainViewModel.h"
#include "abstractions/ui/IWidget.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "application/services/Dispatcher.h"
#include "adapters/ui/win32/controls/TypeSelectorGroup.h"
#include "adapters/ui/win32/panels/StatusPanel.h"
#include "adapters/ui/win32/panels/OptionPanel.h"
#include "adapters/ui/win32/panels/ActionPanel.h"
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

namespace winsetup::adapters::ui {

    class Win32MainWindow final : public abstractions::IWindow {
    public:
        Win32MainWindow(
            std::shared_ptr<abstractions::ILogger> logger,
            std::shared_ptr<abstractions::IMainViewModel> viewModel,
            std::shared_ptr<application::Dispatcher> dispatcher
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

        HWND mhWnd{ nullptr };
        HINSTANCE mhInstance{ nullptr };
        std::shared_ptr<abstractions::ILogger> mLogger;
        std::shared_ptr<abstractions::IMainViewModel> mViewModel;
        std::shared_ptr<application::Dispatcher> mDispatcher;

        TypeSelectorGroup mTypeSelectorGroup;
        RECT mSelectorRect{};
        StatusPanel mStatusPanel;
        OptionPanel mOptionPanel;
        ActionPanel mActionPanel;
        std::vector<abstractions::IWidget*> mWidgets;

        static constexpr int WINDOW_WIDTH = 640;
        static constexpr int WINDOW_HEIGHT = 430;
        static constexpr int TYPE_SELECTOR_GROUP_ID = 100;
        static constexpr UINT_PTR MAIN_TIMER_ID = 2001;
        static constexpr auto CLASS_NAME = L"WinSetupMainWindow";
    };

}
