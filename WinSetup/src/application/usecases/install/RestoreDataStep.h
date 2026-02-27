#pragma once
#include "abstractions/usecases/steps/IRestoreDataStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class RestoreDataStep final : public abstractions::IRestoreDataStep {
        public:
            explicit RestoreDataStep(std::shared_ptr<abstractions::ILogger> logger);
            ~RestoreDataStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
