#include "application/usecases/install/BackupDataStep.h"

namespace winsetup {
    namespace application {

        BackupDataStep::BackupDataStep(std::shared_ptr<abstractions::ILogger> logger)
            : mLogger(std::move(logger))
        {
        }

        domain::Expected<void> BackupDataStep::Execute() {
            if (mLogger) mLogger->Info(L"BackupDataStep: stub.");
            return domain::Expected<void>();
        }

    } // namespace application
} // namespace winsetup
