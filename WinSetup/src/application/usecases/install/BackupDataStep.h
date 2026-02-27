#pragma once
#include "abstractions/usecases/steps/IBackupDataStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class BackupDataStep final : public abstractions::IBackupDataStep {
        public:
            explicit BackupDataStep(std::shared_ptr<abstractions::ILogger> logger);
            ~BackupDataStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
