#pragma once
#include "abstractions/usecases/steps/IApplyImageStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class ApplyImageStep final : public abstractions::IApplyImageStep {
        public:
            explicit ApplyImageStep(std::shared_ptr<abstractions::ILogger> logger);
            ~ApplyImageStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
