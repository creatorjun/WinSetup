// src/main/ServiceRegistration.h
#pragma once

#include <application/core/DIContainer.h>
#include <memory>

namespace winsetup {

    class ServiceRegistration {
    public:
        static void RegisterAllServices(application::DIContainer& container);

    private:
        static void RegisterInfrastructureServices(application::DIContainer& container);
        static void RegisterDomainServices(application::DIContainer& container);
        static void RegisterApplicationServices(application::DIContainer& container);
        static void RegisterPlatformServices(application::DIContainer& container);
        static void RegisterUIServices(application::DIContainer& container);
    };

}
