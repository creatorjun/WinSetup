#include "application/usecases/install/ProvisioningStep.h"

namespace winsetup {
    namespace application {

        ProvisioningStep::ProvisioningStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> ProvisioningStep::Execute() {
            if (mLogger) mLogger->Info(L"ProvisioningStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
