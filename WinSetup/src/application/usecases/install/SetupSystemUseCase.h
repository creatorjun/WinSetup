#pragma once
#include "abstractions/usecases/ISetupSystemUseCase.h"
#include "abstractions/usecases/steps/IBackupDataStep.h"
#include "abstractions/usecases/steps/IFormatPartitionStep.h"
#include "abstractions/usecases/steps/IApplyImageStep.h"
#include "abstractions/usecases/steps/IInstallDriversStep.h"
#include "abstractions/usecases/steps/IRestoreDataStep.h"
#include "abstractions/usecases/steps/IProvisioningStep.h"
#include "abstractions/usecases/steps/IRebootStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include <memory>

namespace winsetup {
    namespace application {

        class SetupSystemUseCase final : public abstractions::ISetupSystemUseCase {
        public:
            explicit SetupSystemUseCase(
                std::shared_ptr<abstractions::IBackupDataStep> backupData,
                std::shared_ptr<abstractions::IFormatPartitionStep> formatPartition,
                std::shared_ptr<abstractions::IApplyImageStep> applyImage,
                std::shared_ptr<abstractions::IInstallDriversStep> installDrivers,
                std::shared_ptr<abstractions::IRestoreDataStep> restoreData,
                std::shared_ptr<abstractions::IProvisioningStep> provisioning,
                std::shared_ptr<abstractions::IRebootStep> reboot,
                std::shared_ptr<abstractions::ILogger> logger);

            ~SetupSystemUseCase() override = default;
            SetupSystemUseCase(const SetupSystemUseCase&) = delete;
            SetupSystemUseCase& operator=(const SetupSystemUseCase&) = delete;

            [[nodiscard]] domain::Expected<void> Execute(
                std::shared_ptr<const domain::SetupConfig> config) override;

        private:
            std::shared_ptr<abstractions::IBackupDataStep>      mBackupData;
            std::shared_ptr<abstractions::IFormatPartitionStep> mFormatPartition;
            std::shared_ptr<abstractions::IApplyImageStep>      mApplyImage;
            std::shared_ptr<abstractions::IInstallDriversStep>  mInstallDrivers;
            std::shared_ptr<abstractions::IRestoreDataStep>     mRestoreData;
            std::shared_ptr<abstractions::IProvisioningStep>    mProvisioning;
            std::shared_ptr<abstractions::IRebootStep>          mReboot;
            std::shared_ptr<abstractions::ILogger>              mLogger;
        };

    } // namespace application
} // namespace winsetup
