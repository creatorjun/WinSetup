// src/main/ServiceRegistration.cpp

#include "ServiceRegistration.h"
#include "../application/core/DIContainer.h"
#include "../adapters/platform/win32/logging/Win32Logger.h"
#include "../abstractions/infrastructure/logging/ILogger.h"

namespace winsetup::main {

    void RegisterServices(std::shared_ptr<application::DIContainer> container) {

        container->Register<
            abstractions::ILogger,
            adapters::platform::Win32Logger
        >(application::ServiceLifetime::Singleton);

    }

}
