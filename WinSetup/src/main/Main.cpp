// src/main/Main.cpp
#include <Windows.h>
#include <application/core/DIContainer.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/ui/IWindow.h>
#include <adapters/ui/win32/Win32MainWindow.h>
#include "ServiceRegistration.h"
#include <memory>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    winsetup::application::DIContainer container;
    winsetup::ServiceRegistration::RegisterAllServices(container);

    auto loggerResult = container.Resolve<winsetup::abstractions::ILogger>();
    if (!loggerResult.HasValue()) {
        ::MessageBoxW(nullptr, L"Failed to resolve logger", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    auto logger = loggerResult.Value();

    auto viewModelResult = container.Resolve<winsetup::abstractions::IMainViewModel>();
    if (!viewModelResult.HasValue()) {
        logger->Error(L"Failed to resolve MainViewModel");
        ::MessageBoxW(nullptr, L"Failed to resolve MainViewModel", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    auto viewModel = viewModelResult.Value();

    std::shared_ptr<winsetup::abstractions::IWindow> mainWindow =
        std::make_shared<winsetup::adapters::ui::Win32MainWindow>(
            logger,
            viewModel
        );

    if (!mainWindow->Create(hInstance, nCmdShow)) {
        logger->Error(L"Failed to create main window");
        ::MessageBoxW(nullptr, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto initResult = viewModel->Initialize();
    if (!initResult.HasValue()) {
        logger->Error(L"Failed to initialize ViewModel: " + initResult.GetError().GetMessage());
    }

    logger->Info(L"Application started successfully");

    MSG msg = {};
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    logger->Info(L"Application terminated");

    return static_cast<int>(msg.wParam);
}
