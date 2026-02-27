#include "application/usecases/install/ApplyImageStep.h"

namespace winsetup {
    namespace application {

        ApplyImageStep::ApplyImageStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> ApplyImageStep::Execute() {
            if (mLogger) mLogger->Info(L"ApplyImageStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
