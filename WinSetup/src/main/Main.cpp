// src/main/Main.cpp
#include <Windows.h>
#include <application/core/DIContainer.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include "ServiceRegistration.h"
#include <memory>

using namespace winsetup;

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    auto container = std::make_shared<application::DIContainer>();

    auto registrationResult = main::ServiceRegistration::RegisterAll(container);
    if (!registrationResult.HasValue()) {
        MessageBoxW(
            nullptr,
            registrationResult.GetError().GetMessage().c_str(),
            L"Initialization Error",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    auto loggerResult = container->Resolve<abstractions::ILogger>();
    if (!loggerResult.HasValue()) {
        MessageBoxW(
            nullptr,
            L"Failed to resolve ILogger",
            L"Error",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    auto logger = loggerResult.Value();
    logger->Info(L"WinSetup started successfully");

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    logger->Info(L"WinSetup shutting down");

    return static_cast<int>(msg.wParam);
}
