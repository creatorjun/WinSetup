#pragma once

#include <memory>
#include <vector>
#include <abstractions/storage/IDiskService.h>
#include <abstractions/storage/IVolumeService.h>
#include <abstractions/storage/IStorageScanner.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/DiskInfo.h>
#include <domain/entities/VolumeInfo.h>

namespace winsetup::application {

    struct DiskAnalysisResult {
        std::vector<domain::DiskInfo> sortedDisks;
        domain::DiskInfo primaryDisk;
        std::optional<domain::DiskInfo> secondaryDisk;
        domain::VolumeInfo systemVolume;
        std::optional<domain::VolumeInfo> dataVolume;
        bool hasExistingWindows{ false };
        bool hasUserData{ false };
    };

    class DiskAnalyzer {
    public:
        DiskAnalyzer(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::IVolumeService> volumeService,
            std::shared_ptr<abstractions::IStorageScanner> storageScanner
        );

        [[nodiscard]] domain::Expected<DiskAnalysisResult>
            AnalyzeSystem(const std::wstring& userProfile) const noexcept;

        [[nodiscard]] domain::Expected<std::vector<domain::DiskInfo>>
            GetSortedDisks() const noexcept;

        [[nodiscard]] domain::Expected<domain::VolumeInfo>
            FindSystemVolume() const noexcept;

        [[nodiscard]] domain::Expected<domain::VolumeInfo>
            FindDataVolume(const std::wstring& userProfile) const noexcept;

        [[nodiscard]] domain::Expected<bool>
            HasExistingWindows() const noexcept;

    private:
        [[nodiscard]] std::vector<domain::DiskInfo>
            FilterRemovableDisks(const std::vector<domain::DiskInfo>& disks) const noexcept;

        [[nodiscard]] std::vector<domain::DiskInfo>
            SortDisksByPriority(std::vector<domain::DiskInfo> disks) const noexcept;

        [[nodiscard]] domain::Expected<std::vector<domain::VolumeInfo>>
            GetVolumesForDisk(const domain::DiskInfo& disk) const noexcept;

        std::shared_ptr<abstractions::IDiskService> diskService_;
        std::shared_ptr<abstractions::IVolumeService> volumeService_;
        std::shared_ptr<abstractions::IStorageScanner> storageScanner_;
    };

}
