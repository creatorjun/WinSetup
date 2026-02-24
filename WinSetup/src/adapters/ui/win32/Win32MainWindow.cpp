// src/adapters/ui/win32/Win32MainWindow.cpp
#include <adapters/ui/win32/Win32MainWindow.h>
#include <adapters/ui/win32/controls/ToggleButton.h>
#include <resources/resource.h>
#include <windowsx.h>
#undef min
#undef max

namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::ILogger>        logger,
        std::shared_ptr<abstractions::IMainViewModel> viewModel
    )
        : mhWnd(nullptr)
        , mhInstance(nullptr)
        , mlogger(std::move(logger))
        , mviewModel(std::move(viewModel))
        , mselectorRect{}
    {
        if (mviewModel)
            mviewModel->AddPropertyChangedHandler(
                [this](const std::wstring& propertyName) {
                    OnViewModelPropertyChanged(propertyName);
                }
            );
    }

    Win32MainWindow::~Win32MainWindow() {
        if (mviewModel) mviewModel->RemoveAllPropertyChangedHandlers();
        if (mhWnd) { DestroyWindow(mhWnd); mhWnd = nullptr; }
    }

    bool Win32MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
        mhInstance = hInstance;
        ToggleButton::Initialize(mhInstance);

        HICON hIcon = LoadIconW(mhInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        HICON hIconSm = LoadIconW(mhInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!hIcon || !hIconSm) {
            if (mlogger) mlogger->Warning(L"Failed to load application icon, using default");
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
        wc.lpszClassName = CLASSNAME;

        if (!RegisterClassExW(&wc)) {
            if (mlogger) mlogger->Error(L"Failed to register window class");
            return false;
        }

        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT  windowRect{ 0, 0, WINDOWWIDTH, WINDOWHEIGHT };
        AdjustWindowRect(&windowRect, dwStyle, FALSE);

        const int adjustedWidth = windowRect.right - windowRect.left;
        const int adjustedHeight = windowRect.bottom - windowRect.top;
        const int posX = (GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) / 2;
        const int posY = (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) / 2;

        const std::wstring title = mviewModel
            ? mviewModel->GetWindowTitle()
            : L"WinSetup  v 1.0";

        mhWnd = CreateWindowExW(
            0, CLASSNAME, title.c_str(), dwStyle,
            posX, posY, adjustedWidth, adjustedHeight,
            nullptr, nullptr, mhInstance, this
        );

        if (!mhWnd) {
            if (mlogger) mlogger->Error(L"Failed to create window");
            return false;
        }

        ShowWindow(mhWnd, nCmdShow);
        UpdateWindow(mhWnd);

        if (mlogger) mlogger->Info(L"Main window created successfully");
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

    void Win32MainWindow::InitializeWidgets() {
        RECT clientRect{};
        GetClientRect(mhWnd, &clientRect);
        const int cw = clientRect.right;
        const int marginH = 16;
        const int statusH = 60;
        const int typeDescH = 40;
        const int gap = 8;
        const int innerPadTop = 28;
        const int innerPadBot = 12;
        const int btnRows = 2;
        const int btnH = 32;
        const int btnGapV = 8;
        const int selectorH = innerPadTop + btnRows * btnH + (btnRows - 1) * btnGapV + innerPadBot;
        const int panelW = cw - marginH * 2;

        const int statusPanelH = statusH + typeDescH + gap;
        mstatusPanel.SetViewModel(mviewModel);
        mstatusPanel.Create(mhWnd, mhInstance, marginH, 0, panelW, statusPanelH);

        const int selectorY = statusPanelH + gap;
        mselectorRect = { marginH, selectorY, cw - marginH, selectorY + selectorH };
        mtypeSelectorGroup.Create(mhWnd, mhInstance, L"설치 유형", TYPESELECTORGROUPID);
        mtypeSelectorGroup.SetRect(mselectorRect);
        mtypeSelectorGroup.SetSelectionChangedCallback(
            [this](const std::wstring& key) {
                if (mviewModel) mviewModel->SetTypeDescription(key);
            }
        );
        RebuildTypeSelector();

        const int optionY = mselectorRect.bottom + gap * 2;
        const int optionH = btnH * 2 + gap;
        moptionPanel.SetViewModel(mviewModel);
        moptionPanel.Create(mhWnd, mhInstance, marginH, optionY, panelW, optionH);

        const int actionY = optionY + optionH + gap;
        const int actionH = btnH + gap * 2 + btnH;
        mactionPanel.SetViewModel(mviewModel);
        mactionPanel.Create(mhWnd, mhInstance, marginH, actionY, panelW, actionH);

        mwidgets.clear();
        mwidgets.push_back(&mstatusPanel);
        mwidgets.push_back(&moptionPanel);
        mwidgets.push_back(&mactionPanel);
    }

    void Win32MainWindow::RebuildTypeSelector() {
        if (!mviewModel || !mhWnd) return;
        const auto types = mviewModel->GetInstallationTypes();
        if (types.empty()) return;
        mtypeSelectorGroup.Rebuild(types);
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
            if (mviewModel) {
                if (mviewModel->IsProcessing())
                    StartTimer();
                else
                    StopTimer();
            }
            mtypeSelectorGroup.SetEnabled(!mviewModel || !mviewModel->IsProcessing());
        }
        for (auto& widget : mwidgets)
            widget->OnPropertyChanged(propertyName);
    }

    void Win32MainWindow::UpdateWindowTitle() {
        if (mhWnd && mviewModel)
            SetWindowTextW(mhWnd, mviewModel->GetWindowTitle().c_str());
    }

    void Win32MainWindow::OnCreate() {
        if (mlogger) mlogger->Debug(L"Window WM_CREATE received");
        InitializeWidgets();
    }

    void Win32MainWindow::OnDestroy() {
        if (mlogger) mlogger->Info(L"Window destroyed");
        StopTimer();
        ToggleButton::Cleanup();
        PostQuitMessage(0);
    }

    void Win32MainWindow::OnPaint() {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(mhWnd, &ps);
        for (auto& widget : mwidgets)
            widget->OnPaint(hdc);
        mtypeSelectorGroup.OnPaint(hdc);
        EndPaint(mhWnd, &ps);
    }

    void Win32MainWindow::OnTimer(WPARAM timerId) {
        if (static_cast<UINT_PTR>(timerId) == MAIN_TIMER_ID && mviewModel)
            mviewModel->TickTimer();
        for (auto& widget : mwidgets)
            widget->OnTimer(static_cast<UINT_PTR>(timerId));
    }

    void Win32MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
        for (auto& widget : mwidgets)
            if (widget->OnCommand(wParam, lParam)) return;
        mtypeSelectorGroup.OnCommand(wParam, lParam);
    }

    LRESULT CALLBACK Win32MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Win32MainWindow* pThis = nullptr;
        if (uMsg == WM_NCCREATE) {
            auto pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
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
        case WM_CREATE:   OnCreate();               return 0;
        case WM_DESTROY:  OnDestroy();              return 0;
        case WM_PAINT:    OnPaint();                return 0;
        case WM_COMMAND:  OnCommand(wParam, lParam); return 0;
        case WM_TIMER:    OnTimer(wParam);          return 0;
        case WM_CLOSE:    DestroyWindow(mhWnd);     return 0;
        default:          return DefWindowProcW(mhWnd, uMsg, wParam, lParam);
        }
    }

}
