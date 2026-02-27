// src/application/usecases/disk/AnalyzeDisksStep.h
#pragma once
#include "abstractions/usecases/steps/IAnalyzeDisksStep.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include "domain/entities/DiskInfo.h"
#include <memory>
#include <vector>

namespace winsetup::application {

    class AnalyzeDisksStep final : public abstractions::IAnalyzeDisksStep {
    public:
        explicit AnalyzeDisksStep(
            std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
            std::shared_ptr<abstractions::ILogger>             logger);

        ~AnalyzeDisksStep() override = default;

        [[nodiscard]] domain::Expected<void> Execute() override;

    private:
        [[nodiscard]] static bool AllRolesAssigned(
            const abstractions::IAnalysisRepository& repo);
        [[nodiscard]] static domain::DiskInfo* FindDiskByIndex(
            std::vector<domain::DiskInfo>& disks, uint32_t index);
        static void AssignByVolumes(
            std::vector<domain::DiskInfo>& disks,
            const abstractions::IAnalysisRepository& repo);
        static void AssignByPriority(
            std::vector<domain::DiskInfo>& disks);

        std::shared_ptr<abstractions::IAnalysisRepository> mAnalysisRepository;
        std::shared_ptr<abstractions::ILogger>             mLogger;
    };

} // namespace winsetup::application
