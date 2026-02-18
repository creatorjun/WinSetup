// src/adapters/ui/win32/panels/ActionPanel.cpp
#include <adapters/ui/win32/panels/ActionPanel.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    ActionPanel::ActionPanel()
        : mViewModel(nullptr)
        , mHParent(nullptr)
    {
    }

    ActionPanel::~ActionPanel() {
        StopTimer();
    }

    void ActionPanel::Create(
        HWND hParent, HINSTANCE hInstance,
        int x, int y, int width, int height)
    {
        mHParent = hParent;

        mBtnStartStop.Create(
            hParent, L"시작",
            x, y,
            width, BTN_HEIGHT,
            ID_BTN_START_STOP, hInstance);

        mBtnStartStop.SetFontSize(15);

        const int progressY = y + BTN_HEIGHT + GAP * 2;

        mProgressBar.Create(
            hParent, hInstance,
            x, progressY,
            width, PROGRESS_H,
            ID_PROGRESS_BAR);

        mProgressBar.Reset();
    }

    void ActionPanel::SetViewModel(
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        mViewModel = std::move(viewModel);
    }

    bool ActionPanel::OnCommand(WPARAM wParam, LPARAM /*lParam*/) {
        if (HIWORD(wParam) != BN_CLICKED)       return false;
        if (LOWORD(wParam) != ID_BTN_START_STOP) return false;
        if (!mViewModel)                         return false;

        const bool nowProcessing = !mViewModel->IsProcessing();
        mViewModel->SetProcessing(nowProcessing);
        return true;
    }

    void ActionPanel::OnTimer(UINT_PTR timerId) {
        if (timerId != TIMER_ID)                       return;
        if (!mViewModel || !mViewModel->IsProcessing()) return;

        mViewModel->TickTimer();

        if (mViewModel->GetProgress() >= 100) {
            StopTimer();
            mViewModel->SetProcessing(false);
        }
    }

    void ActionPanel::SetEnabled(bool enabled) {
        mBtnStartStop.SetEnabled(enabled);
    }

    void ActionPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!mViewModel) return;

        if (propertyName == L"IsProcessing") {
            const bool processing = mViewModel->IsProcessing();
            mBtnStartStop.SetText(processing ? L"중지" : L"시작");
            if (processing) {
                mProgressBar.Reset();
                StartTimer();
            }
            else {
                StopTimer();
                mProgressBar.Reset();
            }
        }
        else if (propertyName == L"Progress") {
            UpdateProgress();
        }
    }

    void ActionPanel::StartTimer() {
        if (mHParent)
            SetTimer(mHParent, TIMER_ID, 1000, nullptr);
    }

    void ActionPanel::StopTimer() {
        if (mHParent)
            KillTimer(mHParent, TIMER_ID);
    }

    void ActionPanel::UpdateProgress() {
        if (!mViewModel) return;
        mProgressBar.SetProgress(mViewModel->GetProgress());
        mProgressBar.SetRemainingSeconds(mViewModel->GetRemainingSeconds());
    }

}
