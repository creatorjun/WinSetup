#include "application/usecases/install/RestoreDataStep.h"

namespace winsetup {
    namespace application {

        RestoreDataStep::RestoreDataStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> RestoreDataStep::Execute() {
            if (mLogger) mLogger->Info(L"RestoreDataStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
