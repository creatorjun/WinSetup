#pragma once

#include <vector>
#include <memory>
#include "../../domain/primitives/Result.h"
#include "../../domain/entities/DiskInfo.h"
#include "../../domain/primitives/Expected.h"
#include "../../domain/entities/PartitionInfo.h"

namespace winsetup::abstractions {

    class IDiskService {
    public:
        virtual ~IDiskService() = default;

        [[nodiscard]] virtual domain::Expected<std::vector<domain::DiskInfo>>
            EnumeratePhysicalDisks() const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<domain::DiskInfo>
            GetDiskInfo(domain::PhysicalDiskId diskId) const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<std::vector<domain::PartitionInfo>>
            GetPartitions(domain::PhysicalDiskId diskId) const noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            CleanDisk(domain::PhysicalDiskId diskId) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            InitializeDisk(
                domain::PhysicalDiskId diskId,
                domain::PartitionStyle style
            ) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            SetDiskOnline(domain::PhysicalDiskId diskId) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            SetDiskOffline(domain::PhysicalDiskId diskId) noexcept = 0;

        [[nodiscard]] virtual domain::Expected<bool>
            IsDiskOnline(domain::PhysicalDiskId diskId) const noexcept = 0;
    };

}
