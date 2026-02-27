#include "application/usecases/install/SetupSystemUseCase.h"

namespace winsetup::application {

    SetupSystemUseCase::SetupSystemUseCase(
        std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger))
    {
    }

    domain::Expected<void> SetupSystemUseCase::Execute(
        std::shared_ptr<const domain::SetupConfig> config)
    {
        if (!config)
            return domain::Error(
                L"SetupSystemUseCase: SetupConfig not provided",
                0,
                domain::ErrorCategory::System);

        if (mLogger) mLogger->Info(L"SetupSystemUseCase: Started (stub).");
        if (mLogger) mLogger->Info(L"SetupSystemUseCase: [1/5] BackupUserData - stub");
        if (mLogger) mLogger->Info(L"SetupSystemUseCase: [2/5] PrepareDisks - stub");
        if (mLogger) mLogger->Info(L"SetupSystemUseCase: [3/5] ApplyImage - stub");
        if (mLogger) mLogger->Info(L"SetupSystemUseCase: [4/5] InjectDrivers - stub");
        if (mLogger) mLogger->Info(L"SetupSystemUseCase: [5/5] RestoreUserData - stub");
        if (mLogger) mLogger->Info(L"SetupSystemUseCase: Completed (stub).");

        return domain::Expected<void>();
    }

} // namespace winsetup::application
