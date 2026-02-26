// src/adapters/ui/win32/panels/StatusPanel.h
#pragma once
#include "abstractions/ui/IWidget.h"
#include "abstractions/ui/IMainViewModel.h"
#include "adapters/platform/win32/memory/UniqueHandle.h"
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class StatusPanel : public abstractions::IWidget {
    public:
        StatusPanel();
        ~StatusPanel() override = default;
        StatusPanel(const StatusPanel&) = delete;
        StatusPanel& operator=(const StatusPanel&) = delete;

        void Create(const CreateParams& params) override;
        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);

        void OnPaint(void* paintContext) override;
        bool OnCommand(uintptr_t wParam, uintptr_t lParam) override { return false; }
        void OnTimer(uintptr_t timerId) override {}
        void SetEnabled(bool enabled) override {}
        void OnPropertyChanged(const std::wstring& propertyName) override;

        [[nodiscard]] bool IsValid() const noexcept override { return mhParent != nullptr; }

    private:
        void DrawStatusText(void* hdc) const;
        void DrawTypeDescription(void* hdc) const;
        void EnsureFonts();

        void* mhParent = nullptr;
        int    mx = 0;
        int    my = 0;
        int    mwidth = 0;
        int    mheight = 0;

        std::shared_ptr<abstractions::IMainViewModel> mviewModel;

        platform::UniqueHandle mFontStatus;
        platform::UniqueHandle mFontDesc;

        static constexpr int STATUSH = 60;
        static constexpr int TYPEDESCH = 40;
        static constexpr int INNERGAP = 8;
    };

} // namespace winsetup::adapters::ui
