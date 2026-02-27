#pragma once
#include "abstractions/usecases/steps/IRebootStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class RebootStep final : public abstractions::IRebootStep {
        public:
            explicit RebootStep(std::shared_ptr<abstractions::ILogger> logger);
            ~RebootStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
