// src/adapters/persistence/analysis/AnalysisRepository.cpp
#include <adapters/persistence/analysis/AnalysisRepository.h>
#include <algorithm>

namespace winsetup::adapters::persistence {

    void AnalysisRepository::StoreSystemInfo(
        std::shared_ptr<const domain::SystemInfo> systemInfo)
    {
        mSystemInfo = std::move(systemInfo);
    }

    void AnalysisRepository::StoreDisks(
        std::shared_ptr<const std::vector<domain::DiskInfo>> disks)
    {
        mDisks = std::move(disks);
    }

    void AnalysisRepository::StoreVolumes(
        std::shared_ptr<const std::vector<domain::VolumeInfo>> volumes)
    {
        mVolumes = std::move(volumes);
    }

    domain::Expected<std::shared_ptr<const domain::SystemInfo>>
        AnalysisRepository::GetSystemInfo() const
    {
        if (!mSystemInfo)
            return domain::Error(
                L"SystemInfo not available. Run AnalyzeSystemUseCase first.",
                0, domain::ErrorCategory::System);
        return mSystemInfo;
    }

    domain::Expected<std::shared_ptr<const std::vector<domain::DiskInfo>>>
        AnalysisRepository::GetDisks() const
    {
        if (!mDisks)
            return domain::Error(
                L"DiskInfo not available. Run AnalyzeSystemUseCase first.",
                0, domain::ErrorCategory::System);
        return mDisks;
    }

    domain::Expected<std::shared_ptr<const std::vector<domain::VolumeInfo>>>
        AnalysisRepository::GetVolumes() const
    {
        if (!mVolumes)
            return domain::Error(
                L"VolumeInfo not available. Run AnalyzeSystemUseCase first.",
                0, domain::ErrorCategory::System);
        return mVolumes;
    }

    std::optional<domain::VolumeInfo> AnalysisRepository::GetSystemVolume() const {
        if (!mVolumes) return std::nullopt;
        auto it = std::find_if(mVolumes->begin(), mVolumes->end(),
            [](const domain::VolumeInfo& v) { return v.IsSystem(); });
        if (it == mVolumes->end()) return std::nullopt;
        return *it;
    }

    std::optional<domain::VolumeInfo> AnalysisRepository::GetDataVolume() const {
        if (!mVolumes) return std::nullopt;
        auto it = std::find_if(mVolumes->begin(), mVolumes->end(),
            [](const domain::VolumeInfo& v) { return v.IsData(); });
        if (it == mVolumes->end()) return std::nullopt;
        return *it;
    }

    std::optional<domain::VolumeInfo> AnalysisRepository::GetBootVolume() const {
        if (!mVolumes) return std::nullopt;
        auto it = std::find_if(mVolumes->begin(), mVolumes->end(),
            [](const domain::VolumeInfo& v) { return v.IsBoot(); });
        if (it == mVolumes->end()) return std::nullopt;
        return *it;
    }

    std::optional<domain::DiskInfo> AnalysisRepository::GetSystemDisk() const {
        if (!mDisks) return std::nullopt;
        auto it = std::find_if(mDisks->begin(), mDisks->end(),
            [](const domain::DiskInfo& d) { return d.IsSystem(); });
        if (it == mDisks->end()) return std::nullopt;
        return *it;
    }

    std::optional<domain::DiskInfo> AnalysisRepository::GetDataDisk() const {
        if (!mDisks) return std::nullopt;
        auto it = std::find_if(mDisks->begin(), mDisks->end(),
            [](const domain::DiskInfo& d) { return d.IsData(); });
        if (it == mDisks->end()) return std::nullopt;
        return *it;
    }

    bool AnalysisRepository::IsLoaded() const noexcept {
        return mSystemInfo != nullptr
            && mDisks != nullptr
            && mVolumes != nullptr;
    }

    void AnalysisRepository::Clear() noexcept {
        mSystemInfo.reset();
        mDisks.reset();
        mVolumes.reset();
    }

}
