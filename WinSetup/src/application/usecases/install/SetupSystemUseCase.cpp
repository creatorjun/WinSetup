#include "application/usecases/install/SetupSystemUseCase.h"

namespace winsetup {
    namespace application {

        SetupSystemUseCase::SetupSystemUseCase(
            std::shared_ptr<abstractions::IBackupDataStep> backupData,
            std::shared_ptr<abstractions::IFormatPartitionStep> formatPartition,
            std::shared_ptr<abstractions::IApplyImageStep> applyImage,
            std::shared_ptr<abstractions::IInstallDriversStep> installDrivers,
            std::shared_ptr<abstractions::IRestoreDataStep> restoreData,
            std::shared_ptr<abstractions::IProvisioningStep> provisioning,
            std::shared_ptr<abstractions::IRebootStep> reboot,
            std::shared_ptr<abstractions::ILogger> logger)
            : mBackupData(std::move(backupData))
            , mFormatPartition(std::move(formatPartition))
            , mApplyImage(std::move(applyImage))
            , mInstallDrivers(std::move(installDrivers))
            , mRestoreData(std::move(restoreData))
            , mProvisioning(std::move(provisioning))
            , mReboot(std::move(reboot))
            , mLogger(std::move(logger))
        {
        }

        domain::Expected<void> SetupSystemUseCase::Execute(
            std::shared_ptr<const domain::SetupConfig> config)
        {
            if (!config)
                return domain::Error(L"SetupSystemUseCase: SetupConfig not provided", 0, domain::ErrorCategory::System);

            if (mLogger) mLogger->Info(L"SetupSystemUseCase: Started.");

            if (mBackupData) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [1/7] BackupData - stub");
                auto result = mBackupData->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mFormatPartition) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [2/7] FormatPartition - stub");
                auto result = mFormatPartition->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mApplyImage) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [3/7] ApplyImage - stub");
                auto result = mApplyImage->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mInstallDrivers) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [4/7] InstallDrivers - stub");
                auto result = mInstallDrivers->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mRestoreData) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [5/7] RestoreData - stub");
                auto result = mRestoreData->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mProvisioning) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [6/7] Provisioning - stub");
                auto result = mProvisioning->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mReboot) {
                if (mLogger) mLogger->Info(L"SetupSystemUseCase: [7/7] Reboot - stub");
                auto result = mReboot->Execute();
                if (!result.HasValue()) return result.GetError();
            }

            if (mLogger) mLogger->Info(L"SetupSystemUseCase: Completed.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
