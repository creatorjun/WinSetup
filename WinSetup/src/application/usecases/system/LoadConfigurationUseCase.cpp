// src/application/usecases/system/LoadConfigurationUseCase.cpp
#include "LoadConfigurationUseCase.h"
#include <adapters/persistence/config/IniConfigRepository.h>

namespace winsetup::application {

    LoadConfigurationUseCase::LoadConfigurationUseCase(
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<domain::SetupConfig>>
        LoadConfigurationUseCase::Execute(const std::wstring& configPath) {
        if (mLogger) {
            mLogger->Info(L"Loading configuration from: " + configPath);
        }

        adapters::persistence::IniConfigRepository repository;
        auto result = repository.LoadConfig(configPath);

        if (!result.HasValue()) {
            if (mLogger) {
                mLogger->Error(L"Failed to load configuration: " + result.GetError().GetMessage());
            }
            return result.GetError();
        }

        auto config = result.Value();

        if (mLogger) {
            mLogger->Info(L"Configuration loaded successfully");
            mLogger->Info(L"  User Profile: " + config->GetUserProfile());
            mLogger->Info(L"  Data Partition: " + std::wstring(config->HasDataPartition() ? L"Yes" : L"No"));
            mLogger->Info(L"  Backup Targets: " + std::to_wstring(config->GetBackupTargets().size()));
            mLogger->Info(L"  Installation Types: " + std::to_wstring(config->GetInstallationTypes().size()));
        }

        return config;
    }

}
