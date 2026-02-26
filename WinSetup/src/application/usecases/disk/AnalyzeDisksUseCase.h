// src/application/usecases/disk/AnalyzeDisksUseCase.h
#pragma once
#include "abstractions/usecases/IAnalyzeDisksUseCase.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include "domain/entities/DiskInfo.h"
#include <memory>
#include <vector>

namespace winsetup::application {

    class AnalyzeDisksUseCase final : public abstractions::IAnalyzeDisksUseCase {
    public:
        explicit AnalyzeDisksUseCase(
            std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
            std::shared_ptr<abstractions::ILogger>             logger
        );
        ~AnalyzeDisksUseCase() override = default;

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

}
