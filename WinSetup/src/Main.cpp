#include <Windows.h>
#include <iostream>
#include <memory>
#include <string>
#include <exception>

#include "infrastructure/windows/MainWindow.h"
#include "infrastructure/composition/DependencyContainer.h"
#include "infrastructure/composition/ServiceLocator.h"
#include "infrastructure/composition/ServiceRegistration.h"
#include "abstractions/logging/ILogger.h"
#include "infrastructure/logging/WindowsLogger.h"

using namespace winsetup;
using namespace winsetup::infrastructure;
using namespace winsetup::abstractions;

namespace {
    constexpr int DEFAULT_WINDOW_WIDTH = 800;
    constexpr int DEFAULT_WINDOW_HEIGHT = 600;
    constexpr const wchar_t* APPLICATION_TITLE = L"WinSetup";
    constexpr const wchar_t* LOG_FILE_PATH = L"log.txt";

    void ShowErrorMessage(const std::wstring& message) noexcept {
        ::MessageBoxW(
            nullptr,
            message.c_str(),
            L"WinSetup - Error",
            MB_ICONERROR | MB_OK | MB_SETFOREGROUND
        );
    }

    void ShowFatalError(const std::wstring& message) noexcept {
        ::MessageBoxW(
            nullptr,
            message.c_str(),
            L"WinSetup - Fatal Error",
            MB_ICONERROR | MB_OK | MB_SYSTEMMODAL
        );
    }

    [[nodiscard]] bool InitializeLogger(DependencyContainer& container) noexcept {
        try {
            auto logger = std::make_shared<WindowsLogger>();

            if (!logger->Initialize(LOG_FILE_PATH)) {
                return false;
            }

            logger->SetMinimumLevel(LogLevel::Debug);

            container.RegisterInstance<ILogger>(logger);

            logger->Info(L"=================================================");
            logger->Info(L"WinSetup Application Starting");
            logger->Info(L"=================================================");

            return true;
        }
        catch (...) {
            return false;
        }
    }

    [[nodiscard]] bool InitializeDependencies(
        DependencyContainer& container,
        std::shared_ptr<ILogger>& logger
    ) noexcept {
        try {
            if (logger) {
                logger->Info(L"Initializing dependency container...");
            }

            ServiceRegistration::RegisterAll(container);

            if (logger) {
                logger->Info(L"Dependency container initialized successfully");
                logger->Info(L"Registered services count: " +
                    std::to_wstring(container.GetRegisteredCount()));
            }

            return true;
        }
        catch (const std::exception& ex) {
            const std::string errorMsg = ex.what();
            const std::wstring wideMsg(errorMsg.begin(), errorMsg.end());
            ShowErrorMessage(L"Failed to initialize dependencies: " + wideMsg);
            return false;
        }
        catch (...) {
            ShowErrorMessage(L"Unknown error during dependency initialization");
            return false;
        }
    }

    [[nodiscard]] bool ValidateServices(
        DependencyContainer& container,
        std::shared_ptr<ILogger>& logger
    ) noexcept {
        if (!logger) {
            ShowErrorMessage(L"Logger is not initialized");
            return false;
        }

        logger->Debug(L"Validating registered services...");

        const bool allValid =
            container.IsRegistered<ILogger>() &&
            container.IsRegistered<ITextEncoder>() &&
            container.IsRegistered<ISystemInfoService>();

        if (!allValid) {
            logger->Error(L"Service validation failed - missing required services");
            ShowErrorMessage(L"Critical services are not registered properly");
            return false;
        }

        logger->Info(L"All critical services validated successfully");
        return true;
    }

    void LogSystemInformation(ILogger& logger) noexcept {
        try {
            SYSTEM_INFO sysInfo;
            ::GetSystemInfo(&sysInfo);

            logger.Info(L"System Information:");
            logger.Info(L"  Processor Count: " + std::to_wstring(sysInfo.dwNumberOfProcessors));
            logger.Info(L"  Page Size: " + std::to_wstring(sysInfo.dwPageSize));
            logger.Info(L"  Processor Architecture: " + std::to_wstring(sysInfo.wProcessorArchitecture));

            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            if (::GlobalMemoryStatusEx(&memInfo)) {
                const uint64_t totalMemoryMB = memInfo.ullTotalPhys / (1024 * 1024);
                const uint64_t availMemoryMB = memInfo.ullAvailPhys / (1024 * 1024);
                logger.Info(L"  Total Physical Memory: " + std::to_wstring(totalMemoryMB) + L" MB");
                logger.Info(L"  Available Memory: " + std::to_wstring(availMemoryMB) + L" MB");
            }

            wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
            if (::GetComputerNameW(computerName, &size)) {
                logger.Info(L"  Computer Name: " + std::wstring(computerName));
            }
        }
        catch (...) {
            logger.Warning(L"Failed to retrieve system information");
        }
    }

    void Cleanup(
        DependencyContainer* container,
        std::shared_ptr<ILogger>& logger
    ) noexcept {
        if (logger) {
            logger->Info(L"=================================================");
            logger->Info(L"WinSetup Application Shutting Down");
            logger->Info(L"=================================================");
            logger->Flush();
        }

        ServiceLocator::Reset();

        if (container) {
            container->Clear();
        }

        logger.reset();
    }
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int exitCode = 0;
    std::shared_ptr<DependencyContainer> container;
    std::shared_ptr<ILogger> logger;

    try {
        container = std::make_shared<DependencyContainer>();

        if (!InitializeLogger(*container)) {
            ShowErrorMessage(L"Failed to initialize logging system");
            return 1;
        }

        logger = container->Resolve<ILogger>();
        if (!logger) {
            ShowErrorMessage(L"Failed to resolve logger from container");
            return 1;
        }

        if (!InitializeDependencies(*container, logger)) {
            if (logger) {
                logger->Fatal(L"Failed to initialize dependencies");
            }
            Cleanup(container.get(), logger);
            return 2;
        }

        if (!ValidateServices(*container, logger)) {
            if (logger) {
                logger->Fatal(L"Service validation failed");
            }
            Cleanup(container.get(), logger);
            return 3;
        }

        ServiceLocator::Initialize(container);

        if (logger) {
            LogSystemInformation(*logger);
            logger->Info(L"Creating main window...");
        }

        auto mainWindow = std::make_unique<MainWindow>();

        if (!mainWindow->Create(hInstance, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, APPLICATION_TITLE)) {
            const std::wstring errorMsg = L"Failed to create main window (Error: " +
                std::to_wstring(::GetLastError()) + L")";

            if (logger) {
                logger->Fatal(errorMsg);
            }

            ShowErrorMessage(errorMsg);
            Cleanup(container.get(), logger);
            return 4;
        }

        if (logger) {
            logger->Info(L"Main window created successfully");
            logger->Info(L"Starting message loop...");
        }

        mainWindow->Show(nShowCmd);

        exitCode = mainWindow->RunMessageLoop();

        if (logger) {
            logger->Info(L"Message loop exited with code: " + std::to_wstring(exitCode));
        }

        mainWindow.reset();

        Cleanup(container.get(), logger);
    }
    catch (const std::exception& ex) {
        const std::string errorMsg = ex.what();
        const std::wstring wideMsg(errorMsg.begin(), errorMsg.end());
        const std::wstring fullMsg = L"Unhandled exception: " + wideMsg;

        if (logger) {
            logger->Fatal(fullMsg);
            logger->Flush();
        }

        ShowFatalError(fullMsg);
        Cleanup(container.get(), logger);
        return 99;
    }
    catch (...) {
        const std::wstring errorMsg = L"Unknown fatal error occurred";

        if (logger) {
            logger->Fatal(errorMsg);
            logger->Flush();
        }

        ShowFatalError(errorMsg);
        Cleanup(container.get(), logger);
        return 100;
    }

    return exitCode;
}
