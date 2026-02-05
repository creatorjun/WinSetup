#pragma once

#include <vector>
#include "../../domain/primitives/Result.h"
#include "../../domain/entities/DiskInfo.h"
#include "../../domain/primitives/Expected.h"
#include "../../domain/entities/PartitionInfo.h"

namespace winsetup::abstractions {

    struct PartitionLayout {
        uint64_t sizeInBytes;
        domain::FileSystemType fileSystem;
        std::wstring label;
        bool isBootable;
    };

    class IPartitionService {
    public:
        virtual ~IPartitionService() = default;

        [[nodiscard]] virtual domain::Expected<domain::PartitionInfo>
            CreatePartition(
                domain::PhysicalDiskId diskId,
                const PartitionLayout& layout
            ) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            DeletePartition(domain::PartitionId partitionId) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            FormatPartition(
                domain::PartitionId partitionId,
                domain::FileSystemType fileSystem,
                const std::wstring& label,
                bool quickFormat = true
            ) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            SetPartitionActive(domain::PartitionId partitionId) noexcept = 0;

        [[nodiscard]] virtual domain::Expected<uint64_t>
            GetMaxPartitionSize(domain::PhysicalDiskId diskId) const noexcept = 0;
    };

}
