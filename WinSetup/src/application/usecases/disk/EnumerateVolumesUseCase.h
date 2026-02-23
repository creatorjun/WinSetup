// src/application/usecases/disk/EnumerateVolumesUseCase.h
#pragma once

#include <abstractions/usecases/IEnumerateVolumesUseCase.h>
#include <abstractions/services/storage/IVolumeService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::application {

    class EnumerateVolumesUseCase final : public abstractions::IEnumerateVolumesUseCase {
    public:
        explicit EnumerateVolumesUseCase(
            std::shared_ptr<abstractions::IVolumeService> volumeService,
            std::shared_ptr<abstractions::ILogger> logger
        );
        ~EnumerateVolumesUseCase() override = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<std::vector<domain::VolumeInfo>>>
            Execute() override;

    private:
        std::shared_ptr<abstractions::IVolumeService> mVolumeService;
        std::shared_ptr<abstractions::ILogger>        mLogger;
    };

}
