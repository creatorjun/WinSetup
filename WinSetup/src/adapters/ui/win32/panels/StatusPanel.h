// src/adapters/ui/win32/panels/StatusPanel.h
#pragma once

#include <abstractions/ui/IWidget.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/ui/win32/controls/TextWidget.h>
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

        void Create(HWND hParent, HINSTANCE hInstance,
            int x, int y, int width, int height) override;

        void SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel);

        void OnPaint(HDC hdc) override;
        bool OnCommand(WPARAM wParam, LPARAM lParam) override { return false; }
        void OnTimer(UINT_PTR timerId) override {}
        void SetEnabled(bool enabled) override {}
        void OnPropertyChanged(const std::wstring& propertyName) override;

        [[nodiscard]] bool IsValid() const noexcept override { return m_hParent != nullptr; }

    private:
        void DrawStatusText(HDC hdc) const;
        void DrawTypeDescription(HDC hdc) const;

        HWND      m_hParent = nullptr;
        int       m_x = 0;
        int       m_y = 0;
        int       m_width = 0;
        int       m_height = 0;

        std::wstring m_statusText;
        std::wstring m_typeDescription;

        std::shared_ptr<abstractions::IMainViewModel> m_viewModel;

        static constexpr int  STATUS_H = 60;
        static constexpr int  TYPE_DESC_H = 40;
        static constexpr int  INNER_GAP = 8;
    };

}
