// src/adapters/ui/win32/panels/ActionPanel.cpp

#include <adapters/ui/win32/panels/ActionPanel.h>
#include <windowsx.h>

namespace winsetup::adapters::ui {

    ActionPanel::ActionPanel()
        : m_viewModel(nullptr)
        , m_hParent(nullptr)
    {
    }

    ActionPanel::~ActionPanel() {
        StopTimer();
    }

    void ActionPanel::Create(
        HWND hParent, HINSTANCE hInstance,
        int x, int y, int width, int height)
    {
        m_hParent = hParent;

        m_btnStartStop.Create(
            hParent, L"시작",
            x, y,
            width, BTN_HEIGHT,
            ID_BTN_START_STOP, hInstance);

        m_btnStartStop.SetFontSize(15);

        const int progressY = y + BTN_HEIGHT + GAP * 2;

        m_progressBar.Create(
            hParent, hInstance,
            x, progressY,
            width, PROGRESS_H,
            ID_PROGRESS_BAR);

        m_progressBar.Reset();
    }

    void ActionPanel::SetViewModel(
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        m_viewModel = std::move(viewModel);
    }

    bool ActionPanel::OnCommand(WPARAM wParam, LPARAM /*lParam*/) {
        if (HIWORD(wParam) != BN_CLICKED) return false;
        if (LOWORD(wParam) != ID_BTN_START_STOP) return false;
        if (!m_viewModel) return false;

        const bool nowProcessing = !m_viewModel->IsProcessing();
        m_viewModel->SetProcessing(nowProcessing);
        return true;
    }

    void ActionPanel::OnTimer(UINT_PTR timerId) {
        if (timerId != TIMER_ID) return;
        if (!m_viewModel || !m_viewModel->IsProcessing()) return;

        m_viewModel->TickTimer();

        if (m_viewModel->GetProgress() >= 100) {
            StopTimer();
            m_viewModel->SetProcessing(false);
        }
    }

    void ActionPanel::SetEnabled(bool enabled) {
        m_btnStartStop.SetEnabled(enabled);
    }

    void ActionPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!m_viewModel) return;

        if (propertyName == L"IsProcessing") {
            const bool processing = m_viewModel->IsProcessing();
            m_btnStartStop.SetText(processing ? L"중지" : L"시작");
            if (processing) {
                m_progressBar.Reset();
                StartTimer();
            }
            else {
                StopTimer();
                m_progressBar.Reset();
            }
        }
        else if (propertyName == L"Progress") {
            UpdateProgress();
        }
    }

    void ActionPanel::StartTimer() {
        if (m_hParent)
            SetTimer(m_hParent, TIMER_ID, 1000, nullptr);
    }

    void ActionPanel::StopTimer() {
        if (m_hParent)
            KillTimer(m_hParent, TIMER_ID);
    }

    void ActionPanel::UpdateProgress() {
        if (!m_viewModel) return;
        m_progressBar.SetProgress(m_viewModel->GetProgress());
        m_progressBar.SetRemainingSeconds(m_viewModel->GetRemainingSeconds());
    }

}
