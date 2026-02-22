// src/adapters/ui/win32/panels/OptionPanel.cpp
#include <adapters/ui/win32/panels/OptionPanel.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    OptionPanel::OptionPanel()
        : mViewModel(nullptr)
    {
    }

    void OptionPanel::Create(
        HWND hParent, HINSTANCE hInstance,
        int x, int y, int width, int height)
    {
        mBtnDataPreserve.Create(
            hParent, L"데이터 보존",
            x, y,
            width, BTN_HEIGHT,
            ID_TOGGLE_DATA_PRESERVE, hInstance);

        mBtnBitlocker.Create(
            hParent, L"BitLocker 설정",
            x, y + BTN_HEIGHT + BTN_GAP,
            width, BTN_HEIGHT,
            ID_TOGGLE_BITLOCKER, hInstance);

        if (mViewModel) {
            mBtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
            mBtnBitlocker.SetChecked(mViewModel->GetBitlockerEnabled());
        }
    }

    void OptionPanel::SetViewModel(
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        mViewModel = std::move(viewModel);
        if (mViewModel && IsValid()) {
            mBtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
            mBtnBitlocker.SetChecked(mViewModel->GetBitlockerEnabled());
        }
    }

    bool OptionPanel::OnCommand(WPARAM wParam, LPARAM /*lParam*/) {
        if (HIWORD(wParam) != BN_CLICKED) return false;
        const int ctrlId = LOWORD(wParam);

        if (ctrlId == ID_TOGGLE_DATA_PRESERVE && mViewModel) {
            mViewModel->SetDataPreservation(mBtnDataPreserve.IsChecked());
            return true;
        }
        if (ctrlId == ID_TOGGLE_BITLOCKER && mViewModel) {
            mViewModel->SetBitlockerEnabled(mBtnBitlocker.IsChecked());
            return true;
        }
        return false;
    }

    void OptionPanel::SetEnabled(bool enabled) {
        mBtnDataPreserve.SetEnabled(enabled);
        mBtnBitlocker.SetEnabled(enabled);
    }

    void OptionPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!mViewModel) return;

        if (propertyName == L"DataPreservation" && mBtnDataPreserve.Handle()) {
            mBtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
            InvalidateRect(mBtnDataPreserve.Handle(), nullptr, TRUE);
        }
        else if (propertyName == L"BitlockerEnabled" && mBtnBitlocker.Handle()) {
            mBtnBitlocker.SetChecked(mViewModel->GetBitlockerEnabled());
            InvalidateRect(mBtnBitlocker.Handle(), nullptr, TRUE);
        }
        else if (propertyName == L"IsProcessing") {
            const bool processing = mViewModel->IsProcessing();
            SetEnabled(!processing);
        }
    }

}
