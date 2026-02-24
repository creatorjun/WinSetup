// src/domain/services/PartitionAnalyzer.cpp
#include "PartitionAnalyzer.h"

namespace winsetup::domain {

    PartitionAnalysisResult PartitionAnalyzer::Analyze(const std::vector<PartitionInfo>& partitions) {
        PartitionAnalysisResult result{};
        result.totalPartitions = static_cast<uint32_t>(partitions.size());

        for (const auto& partition : partitions) {
            if (partition.GetType() == PartitionType::EFI) {
                result.hasEFIPartition = true;
                result.efiPartition = partition;
            }
            else if (partition.GetType() == PartitionType::MSR) {
                result.hasMSRPartition = true;
            }
            else if (CanInstallWindows(partition)) {
                result.hasWindowsPartition = true;
                if (!result.windowsPartition || partition.GetSize() > result.windowsPartition->GetSize()) {
                    result.windowsPartition = partition;
                }
            }
        }

        return result;
    }

    bool PartitionAnalyzer::CanInstallWindows(const PartitionInfo& partition) noexcept {
        return partition.CanContainWindows();
    }

    bool PartitionAnalyzer::IsSystemPartition(const VolumeInfo& volume) noexcept {
        return volume.IsSystem() || volume.IsBoot();
    }

}
