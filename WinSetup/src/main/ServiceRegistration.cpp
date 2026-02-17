// src/main/ServiceRegistration.cpp
#include <main/ServiceRegistration.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/repositories/IConfigRepository.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/persistence/config/IniConfigRepository.h>
#include <adapters/platform/win32/logging/Win32Logger.h>
#include <application/usecases/system/LoadConfigurationUseCase.h>
#include <application/viewmodels/MainViewModel.h>

namespace winsetup {

    void ServiceRegistration::RegisterAllServices(application::DIContainer& container) {
        RegisterInfrastructureServices(container);
        RegisterDomainServices(container);
        RegisterRepositoryServices(container);
        RegisterUseCaseServices(container);
        RegisterApplicationServices(container);
        RegisterPlatformServices(container);
        RegisterUIServices(container);
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

    void ServiceRegistration::RegisterUseCaseServices(application::DIContainer& container) {
        auto repositoryResult = container.Resolve<abstractions::IConfigRepository>();
        if (!repositoryResult.HasValue()) return;

        auto loggerResult = container.Resolve<abstractions::ILogger>();
        if (!loggerResult.HasValue()) return;

        container.RegisterInstance<application::LoadConfigurationUseCase>(
            std::make_shared<application::LoadConfigurationUseCase>(
                repositoryResult.Value(),
                loggerResult.Value()
            )
        );
    }

    void ServiceRegistration::RegisterApplicationServices(application::DIContainer& container) {
        auto useCaseResult = container.Resolve<application::LoadConfigurationUseCase>();
        if (!useCaseResult.HasValue()) return;

        auto loggerResult = container.Resolve<abstractions::ILogger>();
        if (!loggerResult.HasValue()) return;

        container.RegisterInstance<abstractions::IMainViewModel>(
            std::static_pointer_cast<abstractions::IMainViewModel>(
                std::make_shared<application::MainViewModel>(
                    useCaseResult.Value(),
                    loggerResult.Value()
                )
            )
        );
    }

    void ServiceRegistration::RegisterPlatformServices(application::DIContainer& container) {
    }

    void ServiceRegistration::RegisterUIServices(application::DIContainer& container) {
    }

}
