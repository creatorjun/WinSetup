// src/application/usecases/system/LoadConfigurationUseCase.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <memory>
#include <string>

namespace winsetup::application {

    class LoadConfigurationUseCase {
    public:
        explicit LoadConfigurationUseCase(
            std::shared_ptr<abstractions::ILogger> logger
        );
        ~LoadConfigurationUseCase() = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SetupConfig>>
            Execute(const std::wstring& configPath = L"config.ini");

    private:
        std::shared_ptr<abstractions::ILogger> mLogger;
    };

}
