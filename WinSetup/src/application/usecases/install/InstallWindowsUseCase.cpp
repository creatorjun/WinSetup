#include "application/usecases/install/InstallWindowsUseCase.h"

namespace winsetup::application {

    InstallWindowsUseCase::InstallWindowsUseCase(
        std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger))
    {
    }

    domain::Expected<void> InstallWindowsUseCase::Execute(
        std::shared_ptr<const domain::SetupConfig> config)
    {
        if (!config)
            return domain::Error(
                L"InstallWindowsUseCase: SetupConfig not provided",
                0,
                domain::ErrorCategory::System);

        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: Started (stub).");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [1/5] BackupUserData - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [2/5] PrepareDisks - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [3/5] ApplyImage - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [4/5] InjectDrivers - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [5/5] RestoreUserData - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: Completed (stub).");

        return domain::Expected<void>();
    }

} // namespace winsetup::application
