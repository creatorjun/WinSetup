#include <Windows.h>
#include <iostream>
#include <memory>
#include <string>

#include "infrastructure/windows/MainWindow.h"

using namespace winsetup::infrastructure;

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    auto mainWindow = std::make_unique<MainWindow>();

    if (!mainWindow->Create(hInstance, 640, 480, L"WinSetup")) {
        MessageBoxW(
            nullptr,
            L"Failed to create main window",
            L"Error",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    mainWindow->Show(nShowCmd);

    return mainWindow->RunMessageLoop();
}
