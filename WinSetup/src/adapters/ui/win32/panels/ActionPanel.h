// src/adapters/ui/win32/panels/ActionPanel.h
#pragma once

#include <abstractions/ui/IWidget.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/ui/win32/controls/SimpleButton.h>
#include <adapters/ui/win32/Win32ProgressBar.h>
#include <Windows.h>
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class ActionPanel : public abstractions::IWidget {
    public:
        ActionPanel();
        ~ActionPanel() override;

        ActionPanel(const ActionPanel&) = delete;
        ActionPanel& operator=(const ActionPanel&) = delete;

        void Create(HWND hParent, HINSTANCE hInstance,
            int x, int y, int width, int height) override;

        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);

        void OnPaint(HDC hdc) override {}
        bool OnCommand(WPARAM wParam, LPARAM lParam) override;
        void OnTimer(UINT_PTR timerId) override;
        void SetEnabled(bool enabled) override;
        void OnPropertyChanged(const std::wstring& propertyName) override;

        [[nodiscard]] bool IsValid() const noexcept override {
            return m_btnStartStop.Handle() != nullptr;
        }

    private:
        void StartTimer();
        void StopTimer();
        void UpdateProgress();

        std::shared_ptr<abstractions::IMainViewModel> m_viewModel;

        HWND         m_hParent = nullptr;
        SimpleButton m_btnStartStop;
        Win32ProgressBar m_progressBar;

        static constexpr int    BTN_HEIGHT = 44;
        static constexpr int    PROGRESS_H = 36;
        static constexpr int    GAP = 8;
        static constexpr int    ID_BTN_START_STOP = 4002;
        static constexpr int    ID_PROGRESS_BAR = 4003;
        static constexpr UINT_PTR TIMER_ID = 1001;
    };

}
