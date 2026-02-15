// src/domain/services/DiskSortingService.h
#pragma once

#include <vector>
#include "../entities/DiskInfo.h"

namespace winsetup::domain {

    struct FilterAndSortResult {
        std::vector<DiskInfo> installableDisks;
        std::vector<DiskInfo> filteredDisks;
        size_t totalDisks;

        FilterAndSortResult()
            : totalDisks(0)
        {
        }
    };

    class DiskSortingService {
    public:
        [[nodiscard]] static FilterAndSortResult FilterAndSort(const std::vector<DiskInfo>& disks);

        [[nodiscard]] static std::vector<DiskInfo> SortByPriority(std::vector<DiskInfo> disks);

    private:
        [[nodiscard]] static int GetPriority(const DiskInfo& disk) noexcept;
        [[nodiscard]] static bool IsInstallable(const DiskInfo& disk) noexcept;
    };

}
