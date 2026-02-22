// src/adapters/persistence/analysis/AnalysisRepository.cpp
#include <adapters/persistence/analysis/AnalysisRepository.h>

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

    bool AnalysisRepository::IsLoaded() const noexcept
    {
        return mSystemInfo != nullptr
            && mDisks != nullptr
            && mVolumes != nullptr;
    }

    void AnalysisRepository::Clear() noexcept
    {
        mSystemInfo.reset();
        mDisks.reset();
        mVolumes.reset();
    }

}
