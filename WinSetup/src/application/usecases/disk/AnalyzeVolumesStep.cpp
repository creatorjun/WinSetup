// src / application / usecases / disk / AnalyzeVolumesStep.cpp
#include "application/usecases/disk/AnalyzeVolumesStep.h"
#include <algorithm>

namespace winsetup::application {

    AnalyzeVolumesStep::AnalyzeVolumesStep(
        std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
        std::shared_ptr<abstractions::IConfigRepository>   configRepository,
        std::shared_ptr<abstractions::IPathChecker>        pathChecker,
        std::shared_ptr<abstractions::ILogger>             logger)
        : mAnalysisRepository(std::move(analysisRepository))
        , mConfigRepository(std::move(configRepository))
        , mPathChecker(std::move(pathChecker))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<void> AnalyzeVolumesStep::Execute()
    {
        if (!mAnalysisRepository)
            return domain::Error(L"IAnalysisRepository not provided", 0, domain::ErrorCategory::System);
        if (!mConfigRepository)
            return domain::Error(L"IConfigRepository not provided", 0, domain::ErrorCategory::System);
        if (!mPathChecker)
            return domain::Error(L"IPathChecker not provided", 0, domain::ErrorCategory::System);

        auto configResult = mConfigRepository->GetConfig();
        if (!configResult.HasValue())
            return configResult.GetError();

        auto volumeResult = mAnalysisRepository->GetVolumes();
        if (!volumeResult.HasValue())
            return volumeResult.GetError();

        auto diskResult = mAnalysisRepository->GetDisks();
        if (!diskResult.HasValue())
            return diskResult.GetError();

        const std::wstring userProfile = configResult.Value()->GetUserProfile();

        if (mLogger)
            mLogger->Info(L"AnalyzeVolumesStep: Started. UserProfile=" + userProfile);

        std::vector<domain::VolumeInfo> volumes(*volumeResult.Value());
        std::vector<domain::DiskInfo>   disks(*diskResult.Value());

        FilterUsbDevices(disks, volumes);

        const DiskIndexCache cache = BuildDiskIndexCache(volumes);

        if (auto result = AssignVolumeRoles(disks, volumes, userProfile, cache);
            !result.HasValue())
            return result;

        AssignDiskRoles(disks, volumes, cache);

        LogResult(volumes);

        mAnalysisRepository->StoreUpdatedVolumes(std::move(volumes));
        mAnalysisRepository->StoreUpdatedDisks(std::move(disks));

        if (mLogger)
            mLogger->Info(L"AnalyzeVolumesStep: Complete.");

        return domain::Expected<void>();
    }

    void AnalyzeVolumesStep::FilterUsbDevices(
        std::vector<domain::DiskInfo>& disks,
        std::vector<domain::VolumeInfo>& volumes) const
    {
        std::unordered_set<uint32_t> usbDiskIndices;
        for (const auto& disk : disks) {
            if (disk.GetBusType() == domain::BusType::USB)
                usbDiskIndices.insert(disk.GetIndex());
        }

        disks.erase(
            std::remove_if(disks.begin(), disks.end(),
                [](const domain::DiskInfo& d) {
                    return d.GetBusType() == domain::BusType::USB;
                }),
            disks.end());

        volumes.erase(
            std::remove_if(volumes.begin(), volumes.end(),
                [&](const domain::VolumeInfo& vol) {
                    const auto idx = mPathChecker->FindDiskIndexByVolumeGuid(vol.GetVolumePath());
                    return idx.has_value() && usbDiskIndices.count(idx.value()) > 0;
                }),
            volumes.end());
    }

    AnalyzeVolumesStep::DiskIndexCache AnalyzeVolumesStep::BuildDiskIndexCache(
        const std::vector<domain::VolumeInfo>& volumes) const
    {
        DiskIndexCache cache;
        cache.reserve(volumes.size());
        for (const auto& vol : volumes)
            cache.emplace(vol.GetVolumePath(),
                mPathChecker->FindDiskIndexByVolumeGuid(vol.GetVolumePath()));
        return cache;
    }

    domain::Expected<void> AnalyzeVolumesStep::AssignVolumeRoles(
        std::vector<domain::DiskInfo>& disks,
        std::vector<domain::VolumeInfo>& volumes,
        const std::wstring& userProfile,
        const DiskIndexCache& cache) const
    {
        int systemVolIdx = -1;
        for (int i = 0; i < static_cast<int>(volumes.size()); ++i) {
            if (!IsSystemVolume(volumes[i], userProfile))
                continue;
            if (systemVolIdx != -1) {
                if (mLogger)
                    mLogger->Error(
                        L"AnalyzeVolumesStep: Multiple system volumes detected. "
                        L"Ambiguous environment — aborting analysis.");
                return domain::Error(
                    L"Multiple system volumes detected. Cannot determine target.",
                    0,
                    domain::ErrorCategory::Validation);
            }
            systemVolIdx = i;
        }

        if (systemVolIdx >= 0)
            volumes[systemVolIdx].SetIsSystem(true);

        const std::wstring systemVolGuid = (systemVolIdx >= 0)
            ? volumes[systemVolIdx].GetVolumePath()
            : std::wstring{};

        for (auto& vol : volumes) {
            if (vol.IsSystem())
                continue;
            if (vol.GetVolumePath() == systemVolGuid)
                continue;
            if (IsDataVolume(vol, userProfile)) {
                vol.SetIsData(true);
                break;
            }
        }

        if (systemVolIdx >= 0) {
            const std::wstring& systemGuid = volumes[systemVolIdx].GetVolumePath();
            const auto          systemDiskIt = cache.find(systemGuid);
            const auto          systemDiskIdx = (systemDiskIt != cache.end())
                ? systemDiskIt->second
                : std::optional<uint32_t>{};

            if (systemDiskIdx.has_value()) {
                for (auto& disk : disks) {
                    if (disk.GetIndex() == systemDiskIdx.value()) {
                        disk.SetIsSystem(true);
                        break;
                    }
                }

                for (auto& vol : volumes) {
                    if (vol.IsSystem() || vol.IsData())
                        continue;
                    const auto it = cache.find(vol.GetVolumePath());
                    if (it == cache.end() || !it->second.has_value())
                        continue;
                    if (it->second.value() != systemDiskIdx.value())
                        continue;
                    if (IsBootVolume(vol, disks, cache)) {
                        vol.SetIsBoot(true);
                        break;
                    }
                }
            }
        }

        return domain::Expected<void>();
    }

    void AnalyzeVolumesStep::AssignDiskRoles(
        std::vector<domain::DiskInfo>& disks,
        const std::vector<domain::VolumeInfo>& volumes,
        const DiskIndexCache& cache) const
    {
        auto dataVolIt = std::find_if(volumes.begin(), volumes.end(),
            [](const domain::VolumeInfo& v) { return v.IsData(); });
        if (dataVolIt == volumes.end())
            return;

        const auto it = cache.find(dataVolIt->GetVolumePath());
        if (it == cache.end() || !it->second.has_value())
            return;

        for (auto& disk : disks) {
            if (disk.GetIndex() == it->second.value()) {
                disk.SetIsData(true);
                break;
            }
        }
    }

    void AnalyzeVolumesStep::LogResult(const std::vector<domain::VolumeInfo>& volumes) const
    {
        if (!mLogger)
            return;

        const auto logVolume = [&](const domain::VolumeInfo& vol, const std::wstring& role) {
            const std::wstring letter = vol.GetLetter().empty() ? L"-" : vol.GetLetter();
            const std::wstring label = vol.GetLabel().empty() ? L"-" : vol.GetLabel();
            mLogger->Info(L"AnalyzeVolumesStep: [" + role + L"] "
                + letter + L" [" + label + L"] "
                + vol.GetVolumePath());
            };

        bool foundSystem = false, foundData = false, foundBoot = false;

        for (const auto& vol : volumes) {
            if (vol.IsSystem()) { logVolume(vol, L"System"); foundSystem = true; }
            if (vol.IsData()) { logVolume(vol, L"Data");   foundData = true; }
            if (vol.IsBoot()) { logVolume(vol, L"Boot");   foundBoot = true; }
        }

        if (!foundSystem) mLogger->Warning(L"AnalyzeVolumesStep: [System] volume not found");
        if (!foundData)   mLogger->Warning(L"AnalyzeVolumesStep: [Data]   volume not found");
        if (!foundBoot)   mLogger->Warning(L"AnalyzeVolumesStep: [Boot]   volume not found");
    }

    bool AnalyzeVolumesStep::IsSystemVolume(
        const domain::VolumeInfo& volume,
        const std::wstring& userProfile) const noexcept
    {
        const std::wstring& guid = volume.GetVolumePath();
        if (guid.empty())
            return false;
        return mPathChecker->IsDirectory(guid, L"Windows\\System32")
            && mPathChecker->IsDirectory(guid, L"Users\\" + userProfile);
    }

    bool AnalyzeVolumesStep::IsDataVolume(
        const domain::VolumeInfo& volume,
        const std::wstring& userProfile) const noexcept
    {
        const std::wstring& guid = volume.GetVolumePath();
        if (guid.empty())
            return false;
        return mPathChecker->IsDirectory(guid, userProfile + L"\\desktop")
            || mPathChecker->IsDirectory(guid, userProfile + L"\\Documents");
    }

    bool AnalyzeVolumesStep::IsBootVolume(
        const domain::VolumeInfo& volume,
        const std::vector<domain::DiskInfo>& disks,
        const DiskIndexCache& cache) const noexcept
    {
        if (volume.GetFileSystem() != domain::FileSystemType::FAT32)
            return false;

        const auto it = cache.find(volume.GetVolumePath());
        if (it == cache.end() || !it->second.has_value())
            return false;

        const uint32_t volDiskIdx = it->second.value();
        for (const auto& disk : disks) {
            if (disk.GetIndex() != volDiskIdx)
                continue;
            for (const auto& partition : disk.GetPartitions()) {
                if (partition.GetType() == domain::PartitionType::EFI)
                    return true;
            }
        }
        return false;
    }

} // namespace winsetup::application
