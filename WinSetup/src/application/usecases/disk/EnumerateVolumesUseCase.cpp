// src/application/usecases/disk/EnumerateVolumesUseCase.cpp
#include <application/usecases/disk/EnumerateVolumesUseCase.h>

namespace winsetup::application {

    EnumerateVolumesUseCase::EnumerateVolumesUseCase(
        std::shared_ptr<abstractions::IVolumeService> volumeService,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mVolumeService(std::move(volumeService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<std::vector<domain::VolumeInfo>>>
        EnumerateVolumesUseCase::Execute()
    {
        if (!mVolumeService) {
            if (mLogger)
                mLogger->Warning(L"EnumerateVolumesUseCase: IVolumeService not provided, volume list will be empty.");
            return std::make_shared<std::vector<domain::VolumeInfo>>();
        }

        auto result = mVolumeService->EnumerateVolumes();
        if (!result.HasValue()) {
            if (mLogger)
                mLogger->Warning(L"EnumerateVolumesUseCase: Failed to enumerate volumes - " + result.GetError().GetMessage());
            return std::make_shared<std::vector<domain::VolumeInfo>>();
        }

        if (mLogger)
            mLogger->Info(L"EnumerateVolumesUseCase: Volumes found: " + std::to_wstring(result.Value().size()));

        return std::make_shared<std::vector<domain::VolumeInfo>>(std::move(result.Value()));
    }

}
