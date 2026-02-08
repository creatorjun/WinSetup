// src/main/Main.cpp

#include <Windows.h>
#include "ServiceRegistration.h"
#include "../application/core/DIContainer.h"
#include "../adapters/platform/win32/logging/Win32Logger.h"
#include "../abstractions/infrastructure/logging/ILogger.h"
#include <memory>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

    try {
        main::RegisterServices(container);

        auto logger = container->Resolve<abstractions::ILogger>();
        logger->Info(L"WinSetup 시작");

        logger->Info(L"WinSetup 정상 종료");
        return 0;

    }
    catch (const std::exception& e) {
        std::wstring errorMsg = L"초기화 실패: ";
        std::string what(e.what());
        errorMsg += std::wstring(what.begin(), what.end());

        MessageBoxW(
            nullptr,
            errorMsg.c_str(),
            L"WinSetup 오류",
            MB_OK | MB_ICONERROR
        );

        return -1;
    }
    catch (...) {
        MessageBoxW(
            nullptr,
            L"알 수 없는 오류가 발생했습니다.",
            L"WinSetup 오류",
            MB_OK | MB_ICONERROR
        );
        return -1;
    }
}
