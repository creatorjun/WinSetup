// src/application/usecases/disk/SelectTargetVolumeUseCase.cpp
#include <application/usecases/disk/SelectTargetVolumeUseCase.h>
#include <algorithm>

namespace winsetup::application {

    SelectTargetVolumeUseCase::SelectTargetVolumeUseCase(
        std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
        std::shared_ptr<abstractions::IConfigRepository>   configRepository,
        std::shared_ptr<abstractions::IPathChecker>        pathChecker,
        std::shared_ptr<abstractions::ILogger>             logger
    )
        : mAnalysisRepository(std::move(analysisRepository))
        , mConfigRepository(std::move(configRepository))
        , mPathChecker(std::move(pathChecker))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<void> SelectTargetVolumeUseCase::Execute() {
        if (!mAnalysisRepository)
            return domain::Error(L"IAnalysisRepository not provided", 0, domain::ErrorCategory::System);
        if (!mConfigRepository)
            return domain::Error(L"IConfigRepository not provided", 0, domain::ErrorCategory::System);
        if (!mPathChecker)
            return domain::Error(L"IPathChecker not provided", 0, domain::ErrorCategory::System);

        auto configResult = mConfigRepository->GetConfig();
        if (!configResult.HasValue()) return configResult.GetError();

        auto volumeResult = mAnalysisRepository->GetVolumes();
        if (!volumeResult.HasValue()) return volumeResult.GetError();

        auto diskResult = mAnalysisRepository->GetDisks();
        if (!diskResult.HasValue()) return diskResult.GetError();

        const std::wstring userProfile = configResult.Value()->GetUserProfile();

        if (mLogger)
            mLogger->Info(L"SelectTargetVolumeUseCase: Started. UserProfile=" + userProfile);

        std::vector<domain::VolumeInfo> volumes(*volumeResult.Value());
        std::vector<domain::DiskInfo>   disks(*diskResult.Value());

        int systemVolIdx = -1;

        for (int i = 0; i < static_cast<int>(volumes.size()); ++i) {
            if (IsSystemVolume(volumes[i], userProfile)) {
                volumes[i].SetIsSystem(true);
                systemVolIdx = i;
                if (mLogger)
                    mLogger->Info(L"SelectTargetVolumeUseCase: SystemVolume -> "
                        + volumes[i].GetVolumePath());
                break;
            }
        }

        for (auto& vol : volumes) {
            if (vol.IsSystem()) continue;
            if (IsDataVolume(vol, userProfile)) {
                vol.SetIsData(true);
                if (mLogger)
                    mLogger->Info(L"SelectTargetVolumeUseCase: DataVolume -> "
                        + vol.GetVolumePath());
                break;
            }
        }

        if (systemVolIdx >= 0) {
            const std::wstring systemGuid = volumes[systemVolIdx].GetVolumePath();
            const auto systemDiskIdx = mPathChecker->FindDiskIndexByVolumeGuid(systemGuid);

            if (systemDiskIdx.has_value()) {
                for (auto& disk : disks) {
                    if (disk.GetIndex() == systemDiskIdx.value()) {
                        disk.SetIsSystem(true);
                        if (mLogger)
                            mLogger->Info(L"SelectTargetVolumeUseCase: SystemDisk -> Disk["
                                + std::to_wstring(disk.GetIndex()) + L"]");
                        break;
                    }
                }

                for (auto& vol : volumes) {
                    if (vol.IsSystem() || vol.IsData()) continue;
                    const auto volDiskIdx =
                        mPathChecker->FindDiskIndexByVolumeGuid(vol.GetVolumePath());
                    if (!volDiskIdx.has_value()) continue;
                    if (volDiskIdx.value() != systemDiskIdx.value()) continue;
                    if (IsBootVolume(vol)) {
                        vol.SetIsBoot(true);
                        if (mLogger)
                            mLogger->Info(L"SelectTargetVolumeUseCase: BootVolume -> "
                                + vol.GetVolumePath());
                        break;
                    }
                }
            }
        }

        auto dataVolIt = std::find_if(volumes.begin(), volumes.end(),
            [](const domain::VolumeInfo& v) { return v.IsData(); });
        if (dataVolIt != volumes.end()) {
            const auto dataDiskIdx =
                mPathChecker->FindDiskIndexByVolumeGuid(dataVolIt->GetVolumePath());
            if (dataDiskIdx.has_value()) {
                for (auto& disk : disks) {
                    if (disk.GetIndex() == dataDiskIdx.value() && !disk.IsSystem()) {
                        disk.SetIsData(true);
                        if (mLogger)
                            mLogger->Info(L"SelectTargetVolumeUseCase: DataDisk -> Disk["
                                + std::to_wstring(disk.GetIndex()) + L"]");
                        break;
                    }
                }
            }
        }

        mAnalysisRepository->StoreUpdatedVolumes(std::move(volumes));
        mAnalysisRepository->StoreUpdatedDisks(std::move(disks));

        if (mLogger)
            mLogger->Info(L"SelectTargetVolumeUseCase: Complete.");

        return domain::Expected<void>();
    }

    bool SelectTargetVolumeUseCase::IsSystemVolume(
        const domain::VolumeInfo& volume,
        const std::wstring& userProfile
    ) const noexcept {
        const std::wstring& guid = volume.GetVolumePath();
        if (guid.empty()) return false;
        return mPathChecker->IsDirectory(guid, L"Windows\\System32")
            && mPathChecker->IsDirectory(guid, L"Users\\" + userProfile + L"\\AppData");
    }

    bool SelectTargetVolumeUseCase::IsDataVolume(
        const domain::VolumeInfo& volume,
        const std::wstring& userProfile
    ) const noexcept {
        const std::wstring& guid = volume.GetVolumePath();
        if (guid.empty()) return false;
        return mPathChecker->IsDirectory(guid, userProfile + L"\\Desktop")
            && mPathChecker->IsDirectory(guid, userProfile + L"\\Downloads");
    }

    bool SelectTargetVolumeUseCase::IsBootVolume(
        const domain::VolumeInfo& volume
    ) const noexcept {
        const std::wstring& guid = volume.GetVolumePath();
        if (guid.empty()) return false;
        return mPathChecker->IsDirectory(guid, L"EFI");
    }

}
