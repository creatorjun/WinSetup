#pragma once
#include "abstractions/usecases/steps/IProvisioningStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class ProvisioningStep final : public abstractions::IProvisioningStep {
        public:
            explicit ProvisioningStep(std::shared_ptr<abstractions::ILogger> logger);
            ~ProvisioningStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
