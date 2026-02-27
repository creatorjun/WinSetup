#pragma once
#include "abstractions/usecases/IApplyImageUseCase.h"
#include "abstractions/services/storage/IImagingService.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include <memory>

namespace winsetup::application {

    class ApplyImageUseCase final : public abstractions::IApplyImageUseCase {
    public:
        explicit ApplyImageUseCase(
            std::shared_ptr<abstractions::IImagingService> imagingService,
            std::shared_ptr<abstractions::ILogger>         logger);
        ~ApplyImageUseCase() override = default;
        ApplyImageUseCase(const ApplyImageUseCase&) = delete;
        ApplyImageUseCase& operator=(const ApplyImageUseCase&) = delete;

        [[nodiscard]] domain::Expected<void> Execute(
            std::shared_ptr<const domain::SetupConfig> config) override;

    private:
        std::shared_ptr<abstractions::IImagingService> mImagingService;
        std::shared_ptr<abstractions::ILogger>         mLogger;
    };

} // namespace winsetup::application
