// src/application/usecases/system/LoadConfigurationUseCase.h
#pragma once

#include <abstractions/usecases/ILoadConfigurationUseCase.h>
#include <abstractions/repositories/IConfigRepository.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <string>

namespace winsetup::application {

    class LoadConfigurationUseCase : public abstractions::ILoadConfigurationUseCase {
    public:
        explicit LoadConfigurationUseCase(
            std::shared_ptr<abstractions::IConfigRepository> repository,
            std::shared_ptr<abstractions::ILogger>           logger);
        ~LoadConfigurationUseCase() override = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SetupConfig>>
            Execute(const std::wstring& configPath = L"config.ini") override;

    private:
        std::shared_ptr<abstractions::IConfigRepository> mRepository;
        std::shared_ptr<abstractions::ILogger>           mLogger;
    };

}
