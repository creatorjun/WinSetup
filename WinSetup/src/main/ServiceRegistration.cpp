// src/main/ServiceRegistration.cpp
#include <main/ServiceRegistration.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/repositories/IConfigRepository.h>
#include <abstractions/services/platform/ISystemInfoService.h>
#include <abstractions/services/storage/IFileCopyService.h>
#include <abstractions/usecases/ILoadConfigurationUseCase.h>
#include <abstractions/usecases/IAnalyzeSystemUseCase.h>
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/ui/IWindow.h>
#include <adapters/persistence/config/IniConfigRepository.h>
#include <adapters/platform/win32/logging/Win32Logger.h>
#include <adapters/platform/win32/storage/Win32FileCopyService.h>
#include <adapters/platform/win32/system/Win32SystemInfoService.h>
#include <adapters/ui/win32/Win32MainWindow.h>
#include <application/usecases/system/LoadConfigurationUseCase.h>
#include <application/usecases/system/AnalyzeSystemUseCase.h>
#include <application/viewmodels/MainViewModel.h>
#include <stdexcept>

namespace winsetup {

    namespace {
        template <typename TInterface>
        std::shared_ptr<TInterface> ResolveOrThrow(
            application::DIContainer& container,
            const char* serviceName)
        {
            auto result = container.Resolve<TInterface>();
            if (!result.HasValue())
                throw std::runtime_error(
                    std::string("Critical: Failed to resolve ") + serviceName);
            return result.Value();
        }
    }

    void ServiceRegistration::RegisterAllServices(application::DIContainer& container, HINSTANCE hInstance) {
        RegisterInfrastructureServices(container);
        RegisterDomainServices(container);
        RegisterRepositoryServices(container);
        RegisterPlatformServices(container);
        RegisterStorageServices(container);
        RegisterUseCaseServices(container);
        RegisterApplicationServices(container);
        RegisterUIServices(container, hInstance);
    }

    void ServiceRegistration::RegisterInfrastructureServices(application::DIContainer& container) {
        auto logger = std::make_shared<adapters::platform::Win32Logger>(L"log/log.txt");
        container.RegisterInstance<abstractions::ILogger>(
            std::static_pointer_cast<abstractions::ILogger>(logger)
        );
    }

    void ServiceRegistration::RegisterDomainServices(application::DIContainer& container) {
    }

    void ServiceRegistration::RegisterRepositoryServices(application::DIContainer& container) {
        container.RegisterInstance<abstractions::IConfigRepository>(
            std::static_pointer_cast<abstractions::IConfigRepository>(
                std::make_shared<adapters::persistence::IniConfigRepository>()
            )
        );
    }

    void ServiceRegistration::RegisterPlatformServices(application::DIContainer& container) {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");

        container.RegisterInstance<abstractions::ISystemInfoService>(
            std::static_pointer_cast<abstractions::ISystemInfoService>(
                std::make_shared<adapters::platform::Win32SystemInfoService>(logger)
            )
        );
    }

    void ServiceRegistration::RegisterStorageServices(application::DIContainer& container) {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");

        container.RegisterInstance<abstractions::IFileCopyService>(
            std::static_pointer_cast<abstractions::IFileCopyService>(
                std::make_shared<adapters::platform::Win32FileCopyService>(logger)
            )
        );
    }

    void ServiceRegistration::RegisterUseCaseServices(application::DIContainer& container) {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto repo = ResolveOrThrow<abstractions::IConfigRepository>(container, "IConfigRepository");
        auto sysInfo = ResolveOrThrow<abstractions::ISystemInfoService>(container, "ISystemInfoService");

        container.RegisterInstance<abstractions::ILoadConfigurationUseCase>(
            std::static_pointer_cast<abstractions::ILoadConfigurationUseCase>(
                std::make_shared<application::LoadConfigurationUseCase>(repo, logger)
            )
        );

        container.RegisterInstance<abstractions::IAnalyzeSystemUseCase>(
            std::static_pointer_cast<abstractions::IAnalyzeSystemUseCase>(
                std::make_shared<application::AnalyzeSystemUseCase>(sysInfo, logger)
            )
        );
    }

    void ServiceRegistration::RegisterApplicationServices(application::DIContainer& container) {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto loadConfig = ResolveOrThrow<abstractions::ILoadConfigurationUseCase>(container, "ILoadConfigurationUseCase");
        auto analyze = ResolveOrThrow<abstractions::IAnalyzeSystemUseCase>(container, "IAnalyzeSystemUseCase");

        container.RegisterInstance<abstractions::IMainViewModel>(
            std::static_pointer_cast<abstractions::IMainViewModel>(
                std::make_shared<application::MainViewModel>(loadConfig, analyze, logger)
            )
        );
    }

    void ServiceRegistration::RegisterUIServices(application::DIContainer& container, HINSTANCE hInstance) {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto viewModel = ResolveOrThrow<abstractions::IMainViewModel>(container, "IMainViewModel");

        auto window = std::make_shared<adapters::ui::Win32MainWindow>(logger, viewModel);
        window->Create(hInstance, SW_SHOWDEFAULT);

        container.RegisterInstance<abstractions::IWindow>(
            std::static_pointer_cast<abstractions::IWindow>(window)
        );
    }

}
