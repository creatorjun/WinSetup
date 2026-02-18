// src/adapters/ui/win32/panels/ActionPanel.cpp
#include <adapters/ui/win32/panels/ActionPanel.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    ActionPanel::ActionPanel()
        : mViewModel(nullptr)
        , mHParent(nullptr)
    {
    }

    void ActionPanel::Create(HWND hParent, HINSTANCE hInstance, int x, int y, int width, int height) {
        mHParent = hParent;
        mBtnStartStop.Create(hParent, L"시작", x, y, width, BTNHEIGHT, ID_BTN_STARTSTOP, hInstance);
        mBtnStartStop.SetFontSize(15);
        const int progressY = y + BTNHEIGHT + GAP * 2;
        mProgressBar.Create(hParent, hInstance, x, progressY, width, PROGRESSH, ID_PROGRESSBAR);
        mProgressBar.Reset();
    }

    void ActionPanel::SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel) {
        mViewModel = std::move(viewModel);
    }

    bool ActionPanel::OnCommand(WPARAM wParam, LPARAM lParam) {
        if (HIWORD(wParam) != BN_CLICKED || LOWORD(wParam) != ID_BTN_STARTSTOP || !mViewModel)
            return false;
        mViewModel->SetProcessing(!mViewModel->IsProcessing());
        return true;
    }

    void ActionPanel::SetEnabled(bool enabled) {
        mBtnStartStop.SetEnabled(enabled);
    }

    void ActionPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!mViewModel) return;
        if (propertyName == L"IsProcessing") {
            const bool processing = mViewModel->IsProcessing();
            mBtnStartStop.SetText(processing ? L"중단" : L"시작");
            mProgressBar.Reset();
        }
        else if (propertyName == L"Progress" || propertyName == L"RemainingSeconds") {
            UpdateProgress();
        }
    }

    void ActionPanel::UpdateProgress() {
        if (!mViewModel) return;
        mProgressBar.SetProgress(mViewModel->GetProgress());
        mProgressBar.SetRemainingSeconds(mViewModel->GetRemainingSeconds());
    }

}
