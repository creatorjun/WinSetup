#include "application/usecases/install/InstallDriversStep.h"

namespace winsetup {
    namespace application {

        InstallDriversStep::InstallDriversStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> InstallDriversStep::Execute() {
            if (mLogger) mLogger->Info(L"InstallDriversStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
