// src/adapters/ui/win32/panels/ActionPanel.cpp
#include "adapters/ui/win32/panels/ActionPanel.h"
#include <Windows.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    ActionPanel::ActionPanel()
        : mViewModel(nullptr)
        , mHParent(nullptr)
    {
    }

    void ActionPanel::Create(const CreateParams& params)
    {
        mHParent = params.hParent;
        HWND hParent = static_cast<HWND>(params.hParent);
        HINSTANCE hInst = static_cast<HINSTANCE>(params.hInstance);
        const int x = params.x;
        const int y = params.y;
        const int width = params.width;

        mBtnStartStop.Create(hParent, L"시작", x, y, width, BTNHEIGHT, IDBTNSTARTSTOP, hInst);
        mBtnStartStop.SetFontSize(15);
        mBtnStartStop.SetEnabled(false);

        const int progressY = y + BTNHEIGHT + GAP * 2;
        mProgressBar.Create(hParent, hInst, x, progressY, width, PROGRESSH, IDPROGRESSBAR);
        mProgressBar.Reset();
    }

    void ActionPanel::SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        mViewModel = std::move(viewModel);
    }

    bool ActionPanel::OnCommand(uintptr_t wParam, uintptr_t lParam)
    {
        if (HIWORD(wParam) != BN_CLICKED || LOWORD(wParam) != IDBTNSTARTSTOP || !mViewModel)
            return false;
        if (mViewModel->IsProcessing())
            return true;
        mViewModel->StartInstall();
        return true;
    }

    void ActionPanel::SetEnabled(bool enabled)
    {
        mBtnStartStop.SetEnabled(enabled);
    }

    void ActionPanel::OnPropertyChanged(const std::wstring& propertyName)
    {
        if (!mViewModel) return;

        if (propertyName == L"IsProcessing") {
            const bool processing = mViewModel->IsProcessing();
            mBtnStartStop.SetText(processing ? L"진행 중..." : L"시작");
            mBtnStartStop.SetEnabled(!processing);
            mProgressBar.Reset();
        }
        else if (propertyName == L"EnableAllButtons"
            || propertyName == L"EnableButtonsWithoutDataPreserve") {
            SetEnabled(true);
        }
        else if (propertyName == L"DisableAllButtons") {
            SetEnabled(false);
        }
        else if (propertyName == L"Progress" || propertyName == L"RemainingSeconds") {
            UpdateProgress();
        }
    }

    void ActionPanel::UpdateProgress()
    {
        if (!mViewModel) return;
        mProgressBar.SetProgress(mViewModel->GetProgress());
        mProgressBar.SetRemainingSeconds(mViewModel->GetRemainingSeconds());
    }

} // namespace winsetup::adapters::ui
