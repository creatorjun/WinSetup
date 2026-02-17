// src/main/ServiceRegistration.cpp
#include "ServiceRegistration.h"
#include <abstractions/infrastructure/logging/ILogger.h>
#include <abstractions/ui/IMainViewModel.h>
#include <adapters/platform/win32/logging/Win32Logger.h>
#include <application/viewmodels/MainViewModel.h>

namespace winsetup {

    void ServiceRegistration::RegisterAllServices(application::DIContainer& container)
    {
        RegisterInfrastructureServices(container);
        RegisterDomainServices(container);
        RegisterApplicationServices(container);
        RegisterPlatformServices(container);
        RegisterUIServices(container);
    }

    void ServiceRegistration::RegisterInfrastructureServices(application::DIContainer& container)
    {
        auto logger = std::make_shared<adapters::platform::Win32Logger>(L"log/winsetup.log");
        container.RegisterInstance<abstractions::ILogger>(logger);
    }

    void ServiceRegistration::RegisterDomainServices(application::DIContainer& container)
    {
    }

    void ServiceRegistration::RegisterApplicationServices(application::DIContainer& container)
    {
        auto loggerResult = container.Resolve<abstractions::ILogger>();
        if (loggerResult.HasValue()) {
            auto logger = loggerResult.Value();
            auto viewModel = std::make_shared<application::MainViewModel>(logger);
            container.RegisterInstance<abstractions::IMainViewModel>(viewModel);
        }
    }

    void ServiceRegistration::RegisterPlatformServices(application::DIContainer& container)
    {
    }

    void ServiceRegistration::RegisterUIServices(application::DIContainer& container)
    {
    }

}
