// src/main/ServiceRegistration.h
#pragma once

#include <application/core/DIContainer.h>

namespace winsetup {

    class ServiceRegistration {
    public:
        ServiceRegistration() = delete;

        static void RegisterAllServices(application::DIContainer& container);

    private:
        static void RegisterInfrastructureServices(application::DIContainer& container);
        static void RegisterDomainServices(application::DIContainer& container);
        static void RegisterRepositoryServices(application::DIContainer& container);
        static void RegisterUseCaseServices(application::DIContainer& container);
        static void RegisterApplicationServices(application::DIContainer& container);
        static void RegisterPlatformServices(application::DIContainer& container);
        static void RegisterUIServices(application::DIContainer& container);
    };

}
