// src/domain/services/PartitionAnalyzer.h
#pragma once

#include <domain/entities/PartitionInfo.h>
#include <domain/entities/VolumeInfo.h>
#include <domain/valueobjects/PartitionType.h>
#include <vector>
#include <optional>

namespace winsetup::domain {

    class PartitionAnalyzer {
    public:
        struct AnalysisResult {
            bool hasSystemPartition = false;
            bool hasEFIPartition = false;
            bool hasMSRPartition = false;
            bool hasRecoveryPartition = false;
            int systemPartitionIndex = -1;
            int efiPartitionIndex = -1;
            int dataPartitionIndex = -1;
            DiskSize totalUsedSpace;
            DiskSize totalFreeSpace;
        };

        static AnalysisResult AnalyzePartitions(const std::vector<PartitionInfo>& partitions);

        static std::optional<PartitionInfo> FindSystemPartition(const std::vector<PartitionInfo>& partitions);
        static std::optional<PartitionInfo> FindEFIPartition(const std::vector<PartitionInfo>& partitions);
        static std::optional<PartitionInfo> FindLargestPartition(const std::vector<PartitionInfo>& partitions);

        static bool IsGPTLayout(const std::vector<PartitionInfo>& partitions) noexcept;
        static bool IsMBRLayout(const std::vector<PartitionInfo>& partitions) noexcept;
        static bool IsValidWindowsLayout(const std::vector<PartitionInfo>& partitions) noexcept;

        static DiskSize CalculateTotalSize(const std::vector<PartitionInfo>& partitions) noexcept;
    };

}
