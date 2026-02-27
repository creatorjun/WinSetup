#include "application/usecases/install/ApplyImageUseCase.h"

namespace winsetup::application {

    ApplyImageUseCase::ApplyImageUseCase(
        std::shared_ptr<abstractions::IImagingService> imagingService,
        std::shared_ptr<abstractions::ILogger>         logger)
        : mImagingService(std::move(imagingService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<void> ApplyImageUseCase::Execute(
        std::shared_ptr<const domain::SetupConfig> config)
    {
        if (!config)
            return domain::Error(
                L"ApplyImageUseCase: SetupConfig not provided",
                0,
                domain::ErrorCategory::Imaging);

        if (mLogger)
            mLogger->Info(L"ApplyImageUseCase: Started (stub). user=" + config->GetUserProfile());

        if (mLogger)
            mLogger->Info(L"ApplyImageUseCase: Completed (stub).");

        return domain::Expected<void>();
    }

} // namespace winsetup::application
