// src/main/ServiceRegistration.h

#pragma once

#include <memory>

namespace winsetup::application {
    class DIContainer;
}

namespace winsetup::main {

    void RegisterServices(std::shared_ptr<application::DIContainer> container);

}
