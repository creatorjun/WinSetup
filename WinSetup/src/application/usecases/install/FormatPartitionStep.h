#pragma once
#include "abstractions/usecases/steps/IFormatPartitionStep.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include <memory>

namespace winsetup {
    namespace application {

        class FormatPartitionStep final : public abstractions::IFormatPartitionStep {
        public:
            explicit FormatPartitionStep(std::shared_ptr<abstractions::ILogger> logger);
            ~FormatPartitionStep() override = default;

            [[nodiscard]] domain::Expected<void> Execute() override;

        private:
            std::shared_ptr<abstractions::ILogger> mLogger;
        };

    } // namespace application
} // namespace winsetup
