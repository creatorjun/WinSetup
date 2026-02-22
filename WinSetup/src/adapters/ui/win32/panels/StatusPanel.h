// src/adapters/ui/win32/panels/StatusPanel.h
#pragma once

#include <abstractions/ui/IWidget.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <Windows.h>
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class StatusPanel : public abstractions::IWidget {
    public:
        StatusPanel();
        ~StatusPanel() override = default;
        StatusPanel(const StatusPanel&) = delete;
        StatusPanel& operator=(const StatusPanel&) = delete;

        void Create(HWND hParent, HINSTANCE hInstance, int x, int y, int width, int height) override;
        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);

        void OnPaint(HDC hdc) override;
        bool OnCommand(WPARAM wParam, LPARAM lParam) override { return false; }
        void OnTimer(UINT_PTR timerId) override {}
        void SetEnabled(bool enabled) override {}
        void OnPropertyChanged(const std::wstring& propertyName) override;

        [[nodiscard]] bool IsValid() const noexcept override { return mhParent != nullptr; }

    private:
        void DrawStatusText(HDC hdc) const;
        void DrawTypeDescription(HDC hdc) const;
        void EnsureFonts();

        HWND mhParent;
        int  mx;
        int  my;
        int  mwidth;
        int  mheight;

        std::shared_ptr<abstractions::IMainViewModel> mviewModel;

        platform::UniqueHandle mFontStatus;
        platform::UniqueHandle mFontDesc;

        static constexpr int STATUSH = 60;
        static constexpr int TYPEDESCH = 40;
        static constexpr int INNERGAP = 8;
    };

}
