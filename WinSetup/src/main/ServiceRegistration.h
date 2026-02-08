// src/main/ServiceRegistration.h
#pragma once

#include <application/core/DIContainer.h>
#include <domain/primitives/Expected.h>
#include <memory>

namespace winsetup::main {

    class ServiceRegistration {
    public:
        static domain::Expected<void> RegisterAll(
            std::shared_ptr<application::DIContainer> container
        );

    private:
        static void RegisterInfrastructure(application::DIContainer& container);
        static void RegisterServices(application::DIContainer& container);
        static void RegisterUseCases(application::DIContainer& container);
        static void RegisterUI(application::DIContainer& container);
    };

}
