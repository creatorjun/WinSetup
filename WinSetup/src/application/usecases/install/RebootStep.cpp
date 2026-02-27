#include "application/usecases/install/RebootStep.h"

namespace winsetup {
    namespace application {

        RebootStep::RebootStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> RebootStep::Execute() {
            if (mLogger) mLogger->Info(L"RebootStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
