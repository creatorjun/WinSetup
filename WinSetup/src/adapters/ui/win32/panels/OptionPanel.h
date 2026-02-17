// src/adapters/ui/win32/panels/OptionPanel.h
#pragma once

#include <abstractions/ui/IWidget.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/ui/win32/controls/ToggleButton.h>
#include <Windows.h>
#include <memory>
#include <string>

namespace winsetup::adapters::ui {

    class OptionPanel : public abstractions::IWidget {
    public:
        OptionPanel();
        ~OptionPanel() override = default;

        OptionPanel(const OptionPanel&) = delete;
        OptionPanel& operator=(const OptionPanel&) = delete;

        void Create(HWND hParent, HINSTANCE hInstance,
            int x, int y, int width, int height) override;

        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);

        void OnPaint(HDC hdc) override {}
        bool OnCommand(WPARAM wParam, LPARAM lParam) override;
        void OnTimer(UINT_PTR timerId) override {}
        void SetEnabled(bool enabled) override;
        void OnPropertyChanged(const std::wstring& propertyName) override;

        [[nodiscard]] bool IsValid() const noexcept override {
            return m_btnDataPreserve.Handle() != nullptr;
        }

    private:
        std::shared_ptr<abstractions::IMainViewModel> m_viewModel;

        ToggleButton m_btnDataPreserve;
        ToggleButton m_btnBitlocker;

        static constexpr int BTN_HEIGHT = 36;
        static constexpr int BTN_GAP = 8;
        static constexpr int ID_TOGGLE_DATA_PRESERVE = 4000;
        static constexpr int ID_TOGGLE_BITLOCKER = 4001;
    };

}
