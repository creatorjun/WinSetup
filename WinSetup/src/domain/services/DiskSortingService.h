// src/domain/services/DiskSortingService.h
#pragma once

#include <vector>
#include "../entities/DiskInfo.h"

namespace winsetup::domain {

    class DiskSortingService {
    public:
        [[nodiscard]] static std::vector<DiskInfo> SortByPriority(std::vector<DiskInfo> disks);

    private:
        [[nodiscard]] static int GetPriority(const DiskInfo& disk) noexcept;
    };

}
