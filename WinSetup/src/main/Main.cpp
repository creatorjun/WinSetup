#include <Windows.h>
#include "application/core/DIContainer.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "abstractions/ui/IMainViewModel.h"
#include "abstractions/ui/IWindow.h"
#include "main/ServiceRegistration.h"
#include <memory>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    winsetup::application::DIContainer container;
    winsetup::ServiceRegistration::RegisterAllServices(container, hInstance);

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

    auto windowResult = container.Resolve<winsetup::abstractions::IWindow>();
    if (!windowResult.HasValue()) {
        logger->Error(L"Failed to resolve IWindow");
        MessageBoxW(nullptr, L"Failed to resolve IWindow", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto mainWindow = windowResult.Value();
    if (!mainWindow->IsValid()) {
        logger->Error(L"Failed to create main window");
        MessageBoxW(nullptr, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto viewModel = viewModelResult.Value();
    viewModel->InitializeAsync();

    mainWindow->Show();
    logger->Info(L"Application started successfully");

    const bool exitOk = mainWindow->RunMessageLoop();
    logger->Info(L"Application terminated");
    return exitOk ? 0 : 1;
}
