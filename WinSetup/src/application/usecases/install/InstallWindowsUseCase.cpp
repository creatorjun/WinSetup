#include "application/usecases/install/InstallWindowsUseCase.h"

namespace winsetup::application {

    InstallWindowsUseCase::InstallWindowsUseCase(
        std::shared_ptr<abstractions::IApplyImageUseCase> applyImageUseCase,
        std::shared_ptr<abstractions::ILogger>            logger)
        : mApplyImageUseCase(std::move(applyImageUseCase))
        , mLogger(std::move(logger))
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

        if (mApplyImageUseCase) {
            auto result = mApplyImageUseCase->Execute(config);
            if (!result.HasValue()) {
                if (mLogger)
                    mLogger->Error(L"InstallWindowsUseCase: ApplyImage failed - "
                        + result.GetError().GetMessage());
                return result.GetError();
            }
        }
        else {
            if (mLogger)
                mLogger->Info(L"InstallWindowsUseCase: [3/5] ApplyImage - skipped (not registered)");
        }

        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [4/5] InjectDrivers - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: [5/5] RestoreUserData - stub");
        if (mLogger) mLogger->Info(L"InstallWindowsUseCase: Completed (stub).");

        return domain::Expected<void>();
    }

} // namespace winsetup::application
