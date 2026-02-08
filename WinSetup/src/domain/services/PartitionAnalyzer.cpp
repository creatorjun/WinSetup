// src/domain/services/PartitionAnalyzer.cpp
#include <domain/services/PartitionAnalyzer.h>
#include <algorithm>

namespace winsetup::domain {

    PartitionAnalyzer::AnalysisResult PartitionAnalyzer::AnalyzePartitions(
        const std::vector<PartitionInfo>& partitions)
    {
        AnalysisResult result;

        if (partitions.empty()) {
            return result;
        }

        uint64_t totalUsed = 0;

        for (size_t i = 0; i < partitions.size(); ++i) {
            const auto& partition = partitions[i];
            const uint64_t size = partition.GetSize().ToBytes();
            totalUsed += size;

            const PartitionType type = partition.GetType();
            const int idx = static_cast<int>(i);

            switch (type) {
            case PartitionType::System:
                result.hasSystemPartition = true;
                result.systemPartitionIndex = idx;
                break;
            case PartitionType::EFI:
                result.hasEFIPartition = true;
                result.efiPartitionIndex = idx;
                break;
            case PartitionType::MSR:
                result.hasMSRPartition = true;
                break;
            case PartitionType::Recovery:
                result.hasRecoveryPartition = true;
                break;
            case PartitionType::Basic:
                if (result.dataPartitionIndex == -1) {
                    result.dataPartitionIndex = idx;
                }
                break;
            }
        }

        result.totalUsedSpace = DiskSize::FromBytes(totalUsed);
        return result;
    }

    std::optional<PartitionInfo> PartitionAnalyzer::FindSystemPartition(
        const std::vector<PartitionInfo>& partitions)
    {
        auto it = std::find_if(partitions.begin(), partitions.end(),
            [](const auto& p) { return p.IsSystemPartition(); });

        if (it != partitions.end()) {
            return *it;
        }
        return std::nullopt;
    }

    std::optional<PartitionInfo> PartitionAnalyzer::FindEFIPartition(
        const std::vector<PartitionInfo>& partitions)
    {
        auto it = std::find_if(partitions.begin(), partitions.end(),
            [](const auto& p) { return p.GetType() == PartitionType::EFI; });

        if (it != partitions.end()) {
            return *it;
        }
        return std::nullopt;
    }

    std::optional<PartitionInfo> PartitionAnalyzer::FindLargestPartition(
        const std::vector<PartitionInfo>& partitions)
    {
        if (partitions.empty()) {
            return std::nullopt;
        }

        auto it = std::max_element(partitions.begin(), partitions.end(),
            [](const auto& a, const auto& b) {
                return a.GetSize() < b.GetSize();
            });

        return *it;
    }

    bool PartitionAnalyzer::IsGPTLayout(const std::vector<PartitionInfo>& partitions) noexcept {
        return std::any_of(partitions.begin(), partitions.end(),
            [](const auto& p) {
                return p.GetType() == PartitionType::EFI ||
                    p.GetType() == PartitionType::MSR;
            });
    }

    bool PartitionAnalyzer::IsMBRLayout(const std::vector<PartitionInfo>& partitions) noexcept {
        return !IsGPTLayout(partitions);
    }

    bool PartitionAnalyzer::IsValidWindowsLayout(const std::vector<PartitionInfo>& partitions) noexcept {
        auto result = AnalyzePartitions(partitions);

        if (IsGPTLayout(partitions)) {
            return result.hasEFIPartition && result.hasMSRPartition;
        }
        else {
            return result.hasSystemPartition;
        }
    }

    DiskSize PartitionAnalyzer::CalculateTotalSize(const std::vector<PartitionInfo>& partitions) noexcept {
        uint64_t total = 0;
        for (const auto& partition : partitions) {
            total += partition.GetSize().ToBytes();
        }
        return DiskSize::FromBytes(total);
    }

}
