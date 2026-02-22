// src/application/usecases/system/LoadConfigurationUseCase.cpp
#include <application/usecases/system/LoadConfigurationUseCase.h>

namespace winsetup::application {

    LoadConfigurationUseCase::LoadConfigurationUseCase(
        std::shared_ptr<abstractions::IConfigRepository> repository,
        std::shared_ptr<abstractions::ILogger>           logger)
        : mRepository(std::move(repository))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<domain::SetupConfig>>
        LoadConfigurationUseCase::Execute(const std::wstring& configPath) {
        if (!mRepository)
            return domain::Error(L"IConfigRepository not provided", 0,
                domain::ErrorCategory::System);

        if (mRepository->IsLoaded())
            return mRepository->GetConfig();

        if (mLogger)
            mLogger->Info(L"Loading configuration from: " + configPath);

        auto result = mRepository->LoadConfig(configPath);
        if (!result.HasValue()) {
            if (mLogger)
                mLogger->Error(L"Failed to load configuration: " +
                    result.GetError().GetMessage());
            return result.GetError();
        }

        auto config = result.Value();

        if (mLogger) {
            mLogger->Info(L"Configuration loaded successfully");
            mLogger->Info(L"  User Profile      : " + config->GetUserProfile());
            mLogger->Info(L"  Data Partition    : " +
                std::wstring(config->HasDataPartition() ? L"Yes" : L"No"));
            mLogger->Info(L"  Backup Targets    : " +
                std::to_wstring(config->GetBackupTargets().size()));
            mLogger->Info(L"  Installation Types: " +
                std::to_wstring(config->GetInstallationTypes().size()));
            mLogger->Info(L"  BitLocker PIN     : " +
                std::wstring(config->GetBitLockerPin().empty() ? L"Not set" : L"Set"));
            const auto& times = config->GetEstimatedTimes();
            mLogger->Info(L"  Estimated Times   : " +
                std::to_wstring(times.size()) + L" model(s)");
            for (const auto& [model, seconds] : times)
                mLogger->Info(L"    " + model + L" = " + std::to_wstring(seconds) + L"s");
        }

        return config;
    }

}
