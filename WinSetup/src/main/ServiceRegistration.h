#pragma once
#include "application/core/DIContainer.h"
#include <Windows.h>

namespace winsetup {

    class ServiceRegistration {
    public:
        ServiceRegistration() = delete;

        static void RegisterAllServices(
            application::DIContainer& container,
            HINSTANCE hInstance);

    private:
        static void RegisterInfrastructureServices(application::DIContainer& container);
        static void RegisterDomainServices(application::DIContainer& container);
        static void RegisterRepositoryServices(application::DIContainer& container);
        static void RegisterPlatformServices(application::DIContainer& container);
        static void RegisterStorageServices(application::DIContainer& container);
        static void RegisterUseCaseServices(application::DIContainer& container);
        static void RegisterApplicationServices(application::DIContainer& container);
        static void RegisterUIServices(application::DIContainer& container, HINSTANCE hInstance);
    };

} // namespace winsetup
