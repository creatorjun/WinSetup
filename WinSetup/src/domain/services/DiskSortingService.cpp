// src/domain/services/DiskSortingService.cpp
#include "DiskSortingService.h"
#include <algorithm>
#include "../valueobjects/BusType.h"
#include "../valueobjects/DiskType.h"

namespace winsetup::domain {

    FilterAndSortResult DiskSortingService::FilterAndSort(const std::vector<DiskInfo>& disks) {
        FilterAndSortResult result;
        result.totalDisks = disks.size();

        std::vector<DiskInfo> installable;
        std::vector<DiskInfo> filtered;

        installable.reserve(disks.size());
        filtered.reserve(disks.size() / 4);

        for (const auto& disk : disks) {
            if (IsInstallable(disk)) {
                installable.push_back(disk);
            }
            else {
                filtered.push_back(disk);
            }
        }

        result.installableDisks = SortByPriority(std::move(installable));
        result.filteredDisks = std::move(filtered);

        return result;
    }

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

    bool DiskSortingService::IsInstallable(const DiskInfo& disk) noexcept {
        DiskType diskType = disk.GetDiskType();

        if (diskType == DiskType::Removable) {
            return false;
        }

        if (diskType == DiskType::Virtual) {
            return false;
        }

        if (disk.GetSize() < DiskSize::FromGB(16)) {
            return false;
        }

        return true;
    }

}
