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
        ~ActionPanel() override = default;
        ActionPanel(const ActionPanel&) = delete;
        ActionPanel& operator=(const ActionPanel&) = delete;

        void Create(HWND hParent, HINSTANCE hInstance, int x, int y, int width, int height) override;
        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);
        void OnPaint(HDC hdc) override;
        bool OnCommand(WPARAM wParam, LPARAM lParam) override;
        void OnTimer(UINT_PTR timerId) override;
        void SetEnabled(bool enabled) override;
        void OnPropertyChanged(const std::wstring& propertyName) override;
        [[nodiscard]] bool IsValid() const noexcept override { return mHParent != nullptr; }

    private:
        void UpdateProgress();

        std::shared_ptr<abstractions::IMainViewModel> mViewModel;
        HWND        mHParent;
        SimpleButton mBtnStartStop;
        Win32ProgressBar mProgressBar;

        static constexpr int IDBTNSTARTSTOP = 4002;
        static constexpr int IDPROGRESSBAR = 4003;
        static constexpr int BTNHEIGHT = 32;
        static constexpr int PROGRESSH = 32;
        static constexpr int GAP = 8;
    };

}
