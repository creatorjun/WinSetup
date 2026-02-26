#include "adapters/ui/win32/Win32MainWindow.h"
#include "adapters/ui/win32/controls/ToggleButton.h"
#include "resources/resource.h"
#include <windowsx.h>
#undef min
#undef max

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::ILogger> logger,
        std::shared_ptr<abstractions::IMainViewModel> viewModel,
        std::shared_ptr<application::Dispatcher> dispatcher
    )
        : mLogger(std::move(logger))
        , mViewModel(std::move(viewModel))
        , mDispatcher(std::move(dispatcher))
    {
        if (mViewModel) {
            mViewModel->AddPropertyChangedHandler([this](const std::wstring& propertyName) {
                OnViewModelPropertyChanged(propertyName);
                });
        }
    }

    Win32MainWindow::~Win32MainWindow() {
        if (mViewModel) mViewModel->RemoveAllPropertyChangedHandlers();
        if (mhWnd) {
            DestroyWindow(mhWnd);
            mhWnd = nullptr;
        }
    }

    bool Win32MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
        mhInstance = hInstance;
        ToggleButton::Initialize(mhInstance);

        HICON hIcon = LoadIconW(mhInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        HICON hIconSm = LoadIconW(mhInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!hIcon || !hIconSm) {
            if (mLogger) mLogger->Warning(L"Failed to load application icon, using default");
            hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
        }

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = mhInstance;
        wc.hIcon = hIcon;
        wc.hIconSm = hIconSm;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = CLASS_NAME;
        if (!RegisterClassExW(&wc)) {
            if (mLogger) mLogger->Error(L"Failed to register window class");
            return false;
        }

        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT windowRect{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        AdjustWindowRect(&windowRect, dwStyle, FALSE);
        const int adjustedWidth = windowRect.right - windowRect.left;
        const int adjustedHeight = windowRect.bottom - windowRect.top;
        const int posX = (GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) / 2;
        const int posY = (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) / 2;

        const std::wstring title = mViewModel ? mViewModel->GetWindowTitle() : L"WinSetup v1.0";
        mhWnd = CreateWindowExW(
            0, CLASS_NAME, title.c_str(), dwStyle,
            posX, posY, adjustedWidth, adjustedHeight,
            nullptr, nullptr, mhInstance, this);

        if (!mhWnd) {
            if (mLogger) mLogger->Error(L"Failed to create window");
            return false;
        }

        if (mDispatcher) mDispatcher->SetTargetHwnd(mhWnd);

        ShowWindow(mhWnd, nCmdShow);
        UpdateWindow(mhWnd);
        if (mLogger) mLogger->Info(L"Main window created successfully");
        return true;
    }

    void Win32MainWindow::Show() { if (mhWnd) ShowWindow(mhWnd, SW_SHOW); }
    void Win32MainWindow::Hide() { if (mhWnd) ShowWindow(mhWnd, SW_HIDE); }
    bool Win32MainWindow::IsValid() const noexcept { return mhWnd != nullptr; }
    HWND Win32MainWindow::GetHWND() const noexcept { return mhWnd; }

    bool Win32MainWindow::RunMessageLoop() {
        MSG msg{};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return msg.wParam == 0;
    }

    void Win32MainWindow::StartTimer() {
        if (mhWnd) SetTimer(mhWnd, MAIN_TIMER_ID, 1000, nullptr);
    }

    void Win32MainWindow::StopTimer() {
        if (mhWnd) KillTimer(mhWnd, MAIN_TIMER_ID);
    }

    void Win32MainWindow::OnCreate() {
        if (mLogger) mLogger->Debug(L"Window WM_CREATE received");
        InitializeWidgets();
    }

    void Win32MainWindow::OnDestroy() {
        if (mLogger) mLogger->Info(L"Window destroyed");
        StopTimer();
        ToggleButton::Cleanup();
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(mhWnd, &ps);
        for (auto* widget : mWidgets)
            widget->OnPaint(static_cast<void*>(hdc));
        mTypeSelectorGroup.OnPaint(hdc);
        EndPaint(mhWnd, &ps);
    }

    void Win32MainWindow::OnTimer(WPARAM timerId) {
        if (static_cast<UINT_PTR>(timerId) == MAIN_TIMER_ID) {
            if (mViewModel) mViewModel->TickTimer();
        }
        for (auto* widget : mWidgets)
            widget->OnTimer(static_cast<uintptr_t>(timerId));
    }

    void Win32MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
        for (auto* widget : mWidgets) {
            if (widget->OnCommand(static_cast<uintptr_t>(wParam), static_cast<uintptr_t>(lParam)))
                return;
        }
        mTypeSelectorGroup.OnCommand(wParam, lParam);
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Win32MainWindow* pThis = nullptr;
        if (uMsg == WM_NCCREATE) {
            auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = static_cast<Win32MainWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->mhWnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<Win32MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Win32MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_DESTROY:
            OnDestroy();
            return 0;
        case WM_PAINT:
            OnPaint();
            return 0;
        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
        case WM_CLOSE:
            DestroyWindow(mhWnd);
            return 0;
        case application::Dispatcher::WM_DISPATCHER_INVOKE:
            if (mDispatcher) mDispatcher->ProcessPending();
            return 0;
        default:
            return DefWindowProcW(mhWnd, uMsg, wParam, lParam);
        }
    }

    void Win32MainWindow::InitializeWidgets() {
        if (!mhWnd || !mhInstance) return;

        constexpr int marginH = 16;
        constexpr int marginT = 12;
        constexpr int gap = 10;
        const int panelW = WINDOW_WIDTH - marginH * 2;

        // StatusPanel 최상단
        constexpr int statusH = 60 + 8 + 40;
        const int statusY = marginT;
        StatusPanel::CreateParams sp{};
        sp.hParent = mhWnd;
        sp.x = marginH;
        sp.y = statusY;
        sp.width = panelW;
        sp.height = statusH;
        mStatusPanel.Create(sp);
        mStatusPanel.SetViewModel(mViewModel);

        // TypeSelectorGroup
        constexpr int selectorH = 120;
        const int selectorY = statusY + statusH + gap;
        mSelectorRect = { marginH, selectorY, marginH + panelW, selectorY + selectorH };
        mTypeSelectorGroup.Create(mhWnd, mhInstance, L"설치 유형", TYPE_SELECTOR_GROUP_ID);
        mTypeSelectorGroup.SetRect(mSelectorRect);
        mTypeSelectorGroup.SetSelectionChangedCallback([this](const std::wstring& key) {
            if (mViewModel) mViewModel->SetTypeDescription(key);
            });

        // OptionPanel
        constexpr int optionH = 32 + 8 + 32;
        const int optionY = selectorY + selectorH + gap;
        OptionPanel::CreateParams op{};
        op.hParent = mhWnd;
        op.hInstance = mhInstance;
        op.x = marginH;
        op.y = optionY;
        op.width = panelW;
        op.height = optionH;
        mOptionPanel.Create(op);
        mOptionPanel.SetViewModel(mViewModel);

        // ActionPanel
        constexpr int btnH = 32;
        const int actionY = optionY + optionH + gap;
        ActionPanel::CreateParams ap{};
        ap.hParent = mhWnd;
        ap.hInstance = mhInstance;
        ap.x = marginH;
        ap.y = actionY;
        ap.width = panelW;
        ap.height = btnH + gap * 2 + btnH;
        mActionPanel.Create(ap);
        mActionPanel.SetViewModel(mViewModel);

        mWidgets.clear();
        mWidgets.push_back(&mStatusPanel);
        mWidgets.push_back(&mOptionPanel);
        mWidgets.push_back(&mActionPanel);

        RebuildTypeSelector();
    }


    void Win32MainWindow::RebuildTypeSelector() {
        if (!mViewModel || !mhWnd) return;
        const auto domainTypes = mViewModel->GetInstallationTypes();
        if (domainTypes.empty()) return;
        std::vector<InstallationTypeItem> items;
        items.reserve(domainTypes.size());
        for (const auto& t : domainTypes)
            items.push_back({ t.name, t.name });
        mTypeSelectorGroup.Rebuild(items);
    }

    void Win32MainWindow::OnViewModelPropertyChanged(const std::wstring& propertyName) {
        if (propertyName == L"WindowTitle") {
            UpdateWindowTitle();
            return;
        }
        if (propertyName == L"InstallationTypes") {
            RebuildTypeSelector();
            return;
        }
        if (propertyName == L"IsProcessing") {
            if (mViewModel) {
                if (mViewModel->IsProcessing()) StartTimer();
                else StopTimer();
            }
            mTypeSelectorGroup.SetEnabled(!mViewModel || !mViewModel->IsProcessing());
        }
        for (auto* widget : mWidgets)
            widget->OnPropertyChanged(propertyName);
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (mhWnd && mViewModel)
            SetWindowTextW(mhWnd, mViewModel->GetWindowTitle().c_str());
    }

}
