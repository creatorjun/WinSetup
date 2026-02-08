// src/domain/services/DiskSortingService.cpp
#include <domain/services/DiskSortingService.h>

namespace winsetup::domain {

    void DiskSortingService::SortDisks(std::vector<DiskInfo>& disks, SortCriteria criteria) {
        switch (criteria) {
        case SortCriteria::ByTypeAndSize:
            std::sort(disks.begin(), disks.end(), CompareByTypeAndSize);
            break;
        case SortCriteria::ByType:
            std::sort(disks.begin(), disks.end(), CompareByType);
            break;
        case SortCriteria::BySize:
            std::sort(disks.begin(), disks.end(), CompareBySize);
            break;
        case SortCriteria::ByIndex:
            std::sort(disks.begin(), disks.end(), CompareByIndex);
            break;
        }
    }

    std::vector<DiskInfo> DiskSortingService::GetSortedDisks(
        const std::vector<DiskInfo>& disks,
        SortCriteria criteria)
    {
        std::vector<DiskInfo> sorted = disks;
        SortDisks(sorted, criteria);
        return sorted;
    }

    bool DiskSortingService::CompareByTypeAndSize(const DiskInfo& a, const DiskInfo& b) noexcept {
        const int priorityA = a.GetTypePriority();
        const int priorityB = b.GetTypePriority();

        if (priorityA != priorityB) {
            return priorityA < priorityB;
        }

        return a.GetSize() > b.GetSize();
    }

    bool DiskSortingService::CompareByType(const DiskInfo& a, const DiskInfo& b) noexcept {
        return a.GetTypePriority() < b.GetTypePriority();
    }

    bool DiskSortingService::CompareBySize(const DiskInfo& a, const DiskInfo& b) noexcept {
        return a.GetSize() > b.GetSize();
    }

    bool DiskSortingService::CompareByIndex(const DiskInfo& a, const DiskInfo& b) noexcept {
        return a.GetIndex() < b.GetIndex();
    }

}
