// src/main/Main.cpp
#include <Windows.h>
#include <application/core/DIContainer.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/ui/win32/Win32MainWindow.h>
#include <main/ServiceRegistration.h>
#include <memory>

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    winsetup::application::DIContainer container;
    winsetup::ServiceRegistration::RegisterAllServices(container);

    auto loggerResult = container.Resolve<winsetup::abstractions::ILogger>();
    if (!loggerResult.HasValue()) {
        MessageBoxW(nullptr, L"Failed to resolve logger", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    auto logger = loggerResult.Value();

    auto viewModelResult = container.Resolve<winsetup::abstractions::IMainViewModel>();
    if (!viewModelResult.HasValue()) {
        logger->Error(L"Failed to resolve MainViewModel");
        MessageBoxW(nullptr, L"Failed to resolve MainViewModel", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    auto viewModel = viewModelResult.Value();

    auto mainWindow = std::make_shared<winsetup::adapters::ui::Win32MainWindow>(logger, viewModel);
    if (!mainWindow->Create(hInstance, nCmdShow)) {
        logger->Error(L"Failed to create main window");
        MessageBoxW(nullptr, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto initResult = viewModel->Initialize();
    if (!initResult.HasValue())
        logger->Error(L"Failed to initialize ViewModel: " + initResult.GetError().GetMessage());

    logger->Info(L"Application started successfully");

    const bool exitOk = mainWindow->RunMessageLoop();

    logger->Info(L"Application terminated");
    return exitOk ? 0 : 1;
}
