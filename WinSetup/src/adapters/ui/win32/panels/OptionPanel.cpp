// src/adapters/ui/win32/panels/OptionPanel.cpp

#include <adapters/ui/win32/panels/OptionPanel.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    OptionPanel::OptionPanel()
        : m_viewModel(nullptr)
    {
    }

    void OptionPanel::Create(
        HWND hParent, HINSTANCE hInstance,
        int x, int y, int width, int height)
    {
        m_btnDataPreserve.Create(
            hParent, L"데이터 보존",
            x, y,
            width, BTN_HEIGHT,
            ID_TOGGLE_DATA_PRESERVE, hInstance);

        m_btnBitlocker.Create(
            hParent, L"BitLocker 설정",
            x, y + BTN_HEIGHT + BTN_GAP,
            width, BTN_HEIGHT,
            ID_TOGGLE_BITLOCKER, hInstance);

        if (m_viewModel) {
            m_btnDataPreserve.SetChecked(m_viewModel->GetDataPreservation());
            m_btnBitlocker.SetChecked(m_viewModel->GetBitlockerEnabled());
        }
    }

    void OptionPanel::SetViewModel(
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        m_viewModel = std::move(viewModel);
        if (m_viewModel && IsValid()) {
            m_btnDataPreserve.SetChecked(m_viewModel->GetDataPreservation());
            m_btnBitlocker.SetChecked(m_viewModel->GetBitlockerEnabled());
        }
    }

    bool OptionPanel::OnCommand(WPARAM wParam, LPARAM /*lParam*/) {
        if (HIWORD(wParam) != BN_CLICKED) return false;
        const int ctrlId = LOWORD(wParam);

        if (ctrlId == ID_TOGGLE_DATA_PRESERVE && m_viewModel) {
            m_viewModel->SetDataPreservation(m_btnDataPreserve.IsChecked());
            return true;
        }
        if (ctrlId == ID_TOGGLE_BITLOCKER && m_viewModel) {
            m_viewModel->SetBitlockerEnabled(m_btnBitlocker.IsChecked());
            return true;
        }
        return false;
    }

    void OptionPanel::SetEnabled(bool enabled) {
        m_btnDataPreserve.SetEnabled(enabled);
        m_btnBitlocker.SetEnabled(enabled);
    }

    void OptionPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!m_viewModel) return;

        if (propertyName == L"DataPreservation" && m_btnDataPreserve.Handle()) {
            m_btnDataPreserve.SetChecked(m_viewModel->GetDataPreservation());
            InvalidateRect(m_btnDataPreserve.Handle(), nullptr, TRUE);
        }
        else if (propertyName == L"BitlockerEnabled" && m_btnBitlocker.Handle()) {
            m_btnBitlocker.SetChecked(m_viewModel->GetBitlockerEnabled());
            InvalidateRect(m_btnBitlocker.Handle(), nullptr, TRUE);
        }
        else if (propertyName == L"IsProcessing") {
            const bool processing = m_viewModel->IsProcessing();
            SetEnabled(!processing);
        }
    }

}
