// src/domain/services/DiskSortingService.cpp
#include "DiskSortingService.h"
#include <algorithm>
#include "../valueobjects/BusType.h"
#include "../valueobjects/DiskType.h"

namespace winsetup::domain {

    std::vector<DiskInfo> DiskSortingService::SortByPriority(std::vector<DiskInfo> disks) {
        std::stable_sort(disks.begin(), disks.end(), [](const auto& a, const auto& b) {
            int priorityA = GetPriority(a);
            int priorityB = GetPriority(b);

            if (priorityA != priorityB) {
                return priorityA > priorityB;
            }

            return a.GetSize() > b.GetSize();
            });

        return disks;
    }

    int DiskSortingService::GetPriority(const DiskInfo& disk) noexcept {
        BusType busType = disk.GetBusType();
        DiskType diskType = disk.GetDiskType();

        if (busType == BusType::NVME) return 40;
        if (diskType == DiskType::SSD) return 30;
        if (busType == BusType::SATA) return 20;
        if (diskType == DiskType::HDD) return 10;

        return 0;
    }

}
