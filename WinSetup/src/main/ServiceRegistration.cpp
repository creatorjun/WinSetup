// src/main/ServiceRegistration.cpp
#include "ServiceRegistration.h"
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/services/storage/IDiskService.h>
#include <abstractions/services/storage/IVolumeService.h>
#include <abstractions/services/platform/ISystemInfoService.h>
#include <adapters/platform/win32/logging/Win32Logger.h>
#include <adapters/platform/win32/storage/Win32DiskService.h>
#include <adapters/platform/win32/storage/Win32VolumeService.h>
#include <adapters/platform/win32/system/Win32SystemInfoService.h>
#include <application/services/EventBus.h>
#include <application/usecases/system/AnalyzeSystemUseCase.h>
#include <application/usecases/system/LoadConfigurationUseCase.h>
#include <application/usecases/disk/EnumerateDisksUseCase.h>
#include <application/usecases/disk/AnalyzeDisksUseCase.h>

namespace winsetup::main {

    domain::Expected<void> ServiceRegistration::RegisterAll(
        std::shared_ptr<application::DIContainer> container)
    {
        if (!container) {
            return domain::Error{
                L"Container is null",
                0,
                domain::ErrorCategory::System
            };
        }

        try {
            RegisterInfrastructure(*container);
            RegisterServices(*container);
            RegisterUseCases(*container);
            RegisterUI(*container);
            return domain::Expected<void>();
        }
        catch (...) {
            return domain::Error{
                L"Failed to register services",
                0,
                domain::ErrorCategory::System
            };
        }
    }

    void ServiceRegistration::RegisterInfrastructure(application::DIContainer& container) {
        container.Register<abstractions::ILogger, adapters::platform::Win32Logger>(
            application::ServiceLifetime::Singleton
        );
    }

    void ServiceRegistration::RegisterServices(application::DIContainer& container) {
        container.RegisterWithDependencies<
            abstractions::IDiskService,
            adapters::platform::Win32DiskService,
            abstractions::ILogger
        >(application::ServiceLifetime::Singleton);

        container.RegisterWithDependencies<
            abstractions::IVolumeService,
            adapters::platform::Win32VolumeService,
            abstractions::ILogger
        >(application::ServiceLifetime::Singleton);

        container.RegisterWithDependencies<
            abstractions::ISystemInfoService,
            adapters::platform::Win32SystemInfoService,
            abstractions::ILogger
        >(application::ServiceLifetime::Singleton);
    }

    void ServiceRegistration::RegisterUseCases(application::DIContainer& container) {
    }

    void ServiceRegistration::RegisterUI(application::DIContainer& container) {
    }

}
