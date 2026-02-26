// src/adapters/ui/win32/panels/OptionPanel.h
#pragma once
#include "abstractions/ui/IWidget.h"
#include "abstractions/ui/IMainViewModel.h"
#include "adapters/ui/win32/controls/ToggleButton.h"
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class OptionPanel : public abstractions::IWidget {
    public:
        OptionPanel();
        ~OptionPanel() override = default;
        OptionPanel(const OptionPanel&) = delete;
        OptionPanel& operator=(const OptionPanel&) = delete;

        void Create(const CreateParams& params) override;
        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);
        void OnPaint(void* paintContext) override {}
        bool OnCommand(uintptr_t wParam, uintptr_t lParam) override;
        void OnTimer(uintptr_t timerId) override {}
        void SetEnabled(bool enabled) override;
        void OnPropertyChanged(const std::wstring& propertyName) override;
        [[nodiscard]] bool IsValid() const noexcept override { return mBtnDataPreserve.Handle() != nullptr; }

    private:
        std::shared_ptr<abstractions::IMainViewModel> mViewModel;
        ToggleButton mBtnDataPreserve;
        ToggleButton mBtnBitlocker;

        static constexpr int BTNHEIGHT = 32;
        static constexpr int BTNGAP = 8;
        static constexpr int IDTOGGLEDATAPRESERVE = 4000;
        static constexpr int IDTOGGLEBITLOCKER = 4001;
    };

} // namespace winsetup::adapters::ui
