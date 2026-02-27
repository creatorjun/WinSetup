#pragma once
#include "abstractions/usecases/steps/IInstallDriversStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class InstallDriversStep final : public abstractions::IInstallDriversStep {
        public:
            explicit InstallDriversStep(std::shared_ptr<abstractions::ILogger> logger);
            ~InstallDriversStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
