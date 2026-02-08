// src/domain/services/DiskSortingService.h

#pragma once

#include <domain/entities/DiskInfo.h>
#include <vector>
#include <algorithm>

namespace winsetup::domain {

    class DiskSortingService {
    public:
        enum class SortCriteria {
            ByTypeAndSize,
            ByType,
            BySize,
            ByIndex
        };

        static void SortDisks(std::vector<DiskInfo>& disks, SortCriteria criteria = SortCriteria::ByTypeAndSize);

        static std::vector<DiskInfo> GetSortedDisks(
            const std::vector<DiskInfo>& disks,
            SortCriteria criteria = SortCriteria::ByTypeAndSize
        );

    private:
        static bool CompareByTypeAndSize(const DiskInfo& a, const DiskInfo& b) noexcept;
        static bool CompareByType(const DiskInfo& a, const DiskInfo& b) noexcept;
        static bool CompareBySize(const DiskInfo& a, const DiskInfo& b) noexcept;
        static bool CompareByIndex(const DiskInfo& a, const DiskInfo& b) noexcept;
    };

}
