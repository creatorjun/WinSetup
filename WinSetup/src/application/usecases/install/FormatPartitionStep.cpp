#include "application/usecases/install/FormatPartitionStep.h"

namespace winsetup {
    namespace application {

        FormatPartitionStep::FormatPartitionStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> FormatPartitionStep::Execute() {
            if (mLogger) mLogger->Info(L"FormatPartitionStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
