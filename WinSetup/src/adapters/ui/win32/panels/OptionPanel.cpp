#include "adapters/ui/win32/panels/OptionPanel.h"
#include <Windows.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    OptionPanel::OptionPanel()
        : mViewModel(nullptr)
    {
    }

    void OptionPanel::Create(const CreateParams& params)
    {
        HWND hParent = static_cast<HWND>(params.hParent);
        HINSTANCE hInst = static_cast<HINSTANCE>(params.hInstance);
        const int x = params.x;
        const int y = params.y;
        const int width = params.width;

        mBtnDataPreserve.Create(hParent, L"데이터 보존", x, y, width, BTNHEIGHT, IDTOGGLEDATAPRESERVE, hInst);
        mBtnBitlocker.Create(hParent, L"BitLocker", x, y + BTNHEIGHT + BTNGAP, width, BTNHEIGHT, IDTOGGLEBITLOCKER, hInst);

        mBtnDataPreserve.SetEnabled(false);
        mBtnBitlocker.SetEnabled(false);

        if (mViewModel) {
            mBtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
            mBtnBitlocker.SetChecked(mViewModel->GetBitlockerEnabled());
        }
    }

    void OptionPanel::SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        mViewModel = std::move(viewModel);
        if (mViewModel && IsValid()) {
            mBtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
            mBtnBitlocker.SetChecked(mViewModel->GetBitlockerEnabled());
        }
    }

    bool OptionPanel::OnCommand(uintptr_t wParam, uintptr_t lParam)
    {
        if (HIWORD(wParam) != BN_CLICKED) return false;
        const int ctrlId = LOWORD(wParam);
        if (ctrlId == IDTOGGLEDATAPRESERVE && mViewModel) {
            mViewModel->SetDataPreservation(mBtnDataPreserve.IsChecked());
            return true;
        }
        if (ctrlId == IDTOGGLEBITLOCKER && mViewModel) {
            mViewModel->SetBitlockerEnabled(mBtnBitlocker.IsChecked());
            return true;
        }
        return false;
    }

    void OptionPanel::SetEnabled(bool enabled)
    {
        mBtnDataPreserve.SetEnabled(enabled);
        mBtnBitlocker.SetEnabled(enabled && mViewModel && mViewModel->GetBitlockerEnabled());
    }

    void OptionPanel::OnPropertyChanged(const std::wstring& propertyName)
    {
        if (!mViewModel) return;

        if (propertyName == L"DataPreservation") {
            if (mBtnDataPreserve.Handle()) {
                mBtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
                InvalidateRect(mBtnDataPreserve.Handle(), nullptr, TRUE);
            }
        }
        else if (propertyName == L"BitlockerEnabled") {
            const bool enabled = mViewModel->GetBitlockerEnabled();
            if (mBtnBitlocker.Handle()) {
                mBtnBitlocker.SetChecked(enabled);
                mBtnBitlocker.SetEnabled(enabled);
                InvalidateRect(mBtnBitlocker.Handle(), nullptr, TRUE);
            }
        }
        else if (propertyName == L"EnableAllButtons") {
            mBtnDataPreserve.SetEnabled(true);
            mBtnBitlocker.SetEnabled(mViewModel->GetBitlockerEnabled());
        }
        else if (propertyName == L"EnableButtonsWithoutDataPreserve") {
            mBtnDataPreserve.SetEnabled(false);
            mBtnBitlocker.SetEnabled(mViewModel->GetBitlockerEnabled());
        }
        else if (propertyName == L"DisableAllButtons") {
            mBtnDataPreserve.SetEnabled(false);
            mBtnBitlocker.SetEnabled(false);
        }
        else if (propertyName == L"IsProcessing") {
            const bool processing = mViewModel->IsProcessing();
            mBtnDataPreserve.SetEnabled(!processing);
            mBtnBitlocker.SetEnabled(!processing && mViewModel->GetBitlockerEnabled());
        }
    }

} // namespace winsetup::adapters::ui
