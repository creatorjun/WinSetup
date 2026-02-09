// src/abstractions/services/storage/IDiskService.h
#pragma once

#include "../../../domain/primitives/Expected.h"
#include "../../../domain/entities/DiskInfo.h"
#include "../../../domain/entities/PartitionInfo.h"
#include "../../../domain/valueobjects/FileSystemType.h"
#include <vector>
#include <cstdint>

namespace winsetup::abstractions {

    struct PartitionLayout {
        enum class Style {
            MBR,
            GPT
        };

        Style style;
        std::vector<domain::PartitionInfo> partitions;

        [[nodiscard]] bool IsValid() const noexcept {
            return !partitions.empty();
        }
    };

    class IDiskService {
    public:
        virtual ~IDiskService() = default;

        [[nodiscard]] virtual domain::Expected<std::vector<domain::DiskInfo>>
            EnumerateDisks() = 0;

        [[nodiscard]] virtual domain::Expected<domain::DiskInfo>
            GetDiskInfo(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            CleanDisk(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            CreatePartitionLayout(
                uint32_t diskIndex,
                const PartitionLayout& layout
            ) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            FormatPartition(
                uint32_t diskIndex,
                uint32_t partitionIndex,
                domain::FileSystemType fileSystem,
                bool quickFormat = true
            ) = 0;

        [[nodiscard]] virtual domain::Expected<PartitionLayout>
            GetCurrentLayout(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            RestoreLayout(
                uint32_t diskIndex,
                const PartitionLayout& layout
            ) = 0;
    };

}
