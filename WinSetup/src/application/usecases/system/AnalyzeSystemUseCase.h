#pragma once
#include "abstractions/usecases/IAnalyzeSystemUseCase.h"
#include "abstractions/usecases/ILoadConfigurationUseCase.h"
#include "abstractions/usecases/IEnumerateDisksUseCase.h"
#include "abstractions/usecases/IEnumerateVolumesUseCase.h"
#include "abstractions/usecases/IAnalyzeVolumesUseCase.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/services/platform/ISystemInfoService.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include "domain/entities/SystemInfo.h"
#include "domain/entities/DiskInfo.h"
#include "domain/entities/VolumeInfo.h"
#include "domain/valueobjects/BusType.h"
#include "domain/valueobjects/FileSystemType.h"
#include <memory>
#include <vector>

namespace winsetup::application {

    class AnalyzeSystemUseCase final : public abstractions::IAnalyzeSystemUseCase {
    public:
        explicit AnalyzeSystemUseCase(
            std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
            std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfiguration,
            std::shared_ptr<abstractions::IEnumerateDisksUseCase> enumerateDisks,
            std::shared_ptr<abstractions::IEnumerateVolumesUseCase> enumerateVolumes,
            std::shared_ptr<abstractions::IAnalyzeVolumesUseCase> analyzeVolumes,
            std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
            std::shared_ptr<abstractions::ILogger> logger);

        ~AnalyzeSystemUseCase() override = default;

        [[nodiscard]] domain::Expected<void> Execute() override;

    private:
        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SystemInfo>> CollectSystemInfo() const;
        void LogStoredResults(
            const std::vector<domain::DiskInfo>& disks,
            const std::vector<domain::VolumeInfo>& volumes) const;

        [[nodiscard]] static std::wstring BusTypeToString(domain::BusType busType);
        [[nodiscard]] static std::wstring FileSystemTypeToString(domain::FileSystemType fs);

        std::shared_ptr<abstractions::ISystemInfoService> mSystemInfoService;
        std::shared_ptr<abstractions::ILoadConfigurationUseCase> mLoadConfiguration;
        std::shared_ptr<abstractions::IEnumerateDisksUseCase> mEnumerateDisks;
        std::shared_ptr<abstractions::IEnumerateVolumesUseCase> mEnumerateVolumes;
        std::shared_ptr<abstractions::IAnalyzeVolumesUseCase> mAnalyzeVolumes;
        std::shared_ptr<abstractions::IAnalysisRepository> mAnalysisRepository;
        std::shared_ptr<abstractions::ILogger> mLogger;
    };

} // namespace winsetup::application
