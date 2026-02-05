#include "DiskAnalyzer.h"
#include <algorithm>

namespace winsetup::application {

    DiskAnalyzer::DiskAnalyzer(
        std::shared_ptr<abstractions::IDiskService> diskService,
        std::shared_ptr<abstractions::IVolumeService> volumeService,
        std::shared_ptr<abstractions::IStorageScanner> storageScanner
    )
        : diskService_(std::move(diskService))
        , volumeService_(std::move(volumeService))
        , storageScanner_(std::move(storageScanner)) {
    }

    domain::Expected<DiskAnalysisResult>
        DiskAnalyzer::AnalyzeSystem(const std::wstring& userProfile) const noexcept {
        DiskAnalysisResult result;

        auto disksResult = GetSortedDisks();
        if (disksResult.HasError()) [[unlikely]] {
            return domain::Expected<DiskAnalysisResult>::Failure(
                std::move(disksResult).GetError()
            );
        }

        result.sortedDisks = std::move(disksResult).Value();

        if (result.sortedDisks.empty()) [[unlikely]] {
            return domain::Expected<DiskAnalysisResult>::Failure(
                domain::Error("No suitable disks found")
            );
        }

        result.primaryDisk = result.sortedDisks[0];

        if (result.sortedDisks.size() > 1) {
            result.secondaryDisk = result.sortedDisks[1];
        }

        auto systemVolumeResult = FindSystemVolume();
        if (systemVolumeResult.HasValue()) [[likely]] {
            result.systemVolume = std::move(systemVolumeResult).Value();
            result.hasExistingWindows = true;
        }

        auto dataVolumeResult = FindDataVolume(userProfile);
        if (dataVolumeResult.HasValue()) [[likely]] {
            result.dataVolume = std::move(dataVolumeResult).Value();
            result.hasUserData = true;
        }

        return domain::Expected<DiskAnalysisResult>::Success(std::move(result));
    }

    domain::Expected<std::vector<domain::DiskInfo>>
        DiskAnalyzer::GetSortedDisks() const noexcept {
        auto disksResult = diskService_->EnumeratePhysicalDisks();
        if (disksResult.HasError()) [[unlikely]] {
            return domain::Expected<std::vector<domain::DiskInfo>>::Failure(
                std::move(disksResult).GetError()
            );
        }

        auto disks = std::move(disksResult).Value();

        auto filteredDisks = FilterRemovableDisks(disks);

        auto sortedDisks = SortDisksByPriority(std::move(filteredDisks));

        return domain::Expected<std::vector<domain::DiskInfo>>::Success(
            std::move(sortedDisks)
        );
    }

    domain::Expected<domain::VolumeInfo>
        DiskAnalyzer::FindSystemVolume() const noexcept {
        auto volumesResult = volumeService_->EnumerateVolumes();
        if (volumesResult.HasError()) [[unlikely]] {
            return domain::Expected<domain::VolumeInfo>::Failure(
                std::move(volumesResult).GetError()
            );
        }

        auto volumes = std::move(volumesResult).Value();

        abstractions::ScanResult bestResult;
        std::optional<domain::VolumeInfo> bestVolume;
        int bestScore = 0;

        for (const auto& volume : volumes) {
            if (!volume.HasDriveLetter()) continue;

            auto scanResult = storageScanner_->ScanVolume(volume, L"");
            if (scanResult.HasError()) continue;

            auto result = scanResult.Value();
            if (result.IsLikelySystemVolume() && result.score > bestScore) [[unlikely]] {
                bestScore = result.score;
                bestResult = result;
                bestVolume = volume;
            }
        }

        if (!bestVolume.has_value()) [[unlikely]] {
            return domain::Expected<domain::VolumeInfo>::Failure(
                domain::Error("No system volume found")
            );
        }

        return domain::Expected<domain::VolumeInfo>::Success(
            std::move(bestVolume.value())
        );
    }

    domain::Expected<domain::VolumeInfo>
        DiskAnalyzer::FindDataVolume(const std::wstring& userProfile) const noexcept {
        auto volumesResult = volumeService_->EnumerateVolumes();
        if (volumesResult.HasError()) [[unlikely]] {
            return domain::Expected<domain::VolumeInfo>::Failure(
                std::move(volumesResult).GetError()
            );
        }

        auto volumes = std::move(volumesResult).Value();

        abstractions::ScanResult bestResult;
        std::optional<domain::VolumeInfo> bestVolume;
        int bestScore = 0;

        for (const auto& volume : volumes) {
            if (!volume.HasDriveLetter()) continue;

            auto scanResult = storageScanner_->ScanVolume(volume, userProfile);
            if (scanResult.HasError()) continue;

            auto result = scanResult.Value();
            if (result.IsLikelyDataVolume() && result.score > bestScore) [[unlikely]] {
                bestScore = result.score;
                bestResult = result;
                bestVolume = volume;
            }
        }

        if (!bestVolume.has_value()) [[unlikely]] {
            return domain::Expected<domain::VolumeInfo>::Failure(
                domain::Error("No data volume found")
            );
        }

        return domain::Expected<domain::VolumeInfo>::Success(
            std::move(bestVolume.value())
        );
    }

    domain::Expected<bool>
        DiskAnalyzer::HasExistingWindows() const noexcept {
        auto systemVolumeResult = FindSystemVolume();
        return domain::Expected<bool>::Success(systemVolumeResult.HasValue());
    }

    std::vector<domain::DiskInfo>
        DiskAnalyzer::FilterRemovableDisks(
            const std::vector<domain::DiskInfo>& disks
        ) const noexcept {
        std::vector<domain::DiskInfo> filtered;
        filtered.reserve(disks.size());

        std::copy_if(
            disks.begin(),
            disks.end(),
            std::back_inserter(filtered),
            [](const domain::DiskInfo& disk) {
                return !disk.IsRemovable();
            }
        );

        return filtered;
    }

    std::vector<domain::DiskInfo>
        DiskAnalyzer::SortDisksByPriority(
            std::vector<domain::DiskInfo> disks
        ) const noexcept {
        std::sort(disks.begin(), disks.end(),
            [](const domain::DiskInfo& a, const domain::DiskInfo& b) {
                return a.CompareForInstallation(b) == std::strong_ordering::greater;
            }
        );

        return disks;
    }

    domain::Expected<std::vector<domain::VolumeInfo>>
        DiskAnalyzer::GetVolumesForDisk(
            const domain::DiskInfo& disk
        ) const noexcept {
        auto partitionsResult = diskService_->GetPartitions(disk.GetId());
        if (partitionsResult.HasError()) [[unlikely]] {
            return domain::Expected<std::vector<domain::VolumeInfo>>::Failure(
                std::move(partitionsResult).GetError()
            );
        }

        std::vector<domain::VolumeInfo> volumes;
        auto partitions = std::move(partitionsResult).Value();

        for (const auto& partition : partitions) {
            if (!partition.HasVolume()) continue;
            volumes.push_back(partition.GetVolume().value());
        }

        return domain::Expected<std::vector<domain::VolumeInfo>>::Success(
            std::move(volumes)
        );
    }

}
