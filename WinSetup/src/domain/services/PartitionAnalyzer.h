// src/domain/services/PartitionAnalyzer.h
#pragma once

#include "../entities/PartitionInfo.h"
#include "../entities/VolumeInfo.h"
#include <vector>
#include <optional>

namespace winsetup::domain {

    struct PartitionAnalysisResult {
        bool hasWindowsPartition;
        bool hasEFIPartition;
        bool hasMSRPartition;
        std::optional<PartitionInfo> windowsPartition;
        std::optional<PartitionInfo> efiPartition;
        uint32_t totalPartitions;
    };

    class PartitionAnalyzer {
    public:
        [[nodiscard]] static PartitionAnalysisResult Analyze(const std::vector<PartitionInfo>& partitions);
        [[nodiscard]] static bool CanInstallWindows(const PartitionInfo& partition) noexcept;
        [[nodiscard]] static bool IsSystemPartition(const VolumeInfo& volume) noexcept;
    };

}
