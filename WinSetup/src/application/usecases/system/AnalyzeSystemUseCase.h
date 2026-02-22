// src/application/usecases/system/AnalyzeSystemUseCase.h
#pragma once

#include <abstractions/usecases/IAnalyzeSystemUseCase.h>
#include <abstractions/repositories/IAnalysisRepository.h>
#include <abstractions/services/platform/ISystemInfoService.h>
#include <abstractions/services/storage/IDiskService.h>
#include <abstractions/services/storage/IVolumeService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <domain/entities/DiskInfo.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::application {

    class AnalyzeSystemUseCase final : public abstractions::IAnalyzeSystemUseCase {
    public:
        explicit AnalyzeSystemUseCase(
            std::shared_ptr<abstractions::ISystemInfoService>  systemInfoService,
            std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
            std::shared_ptr<abstractions::ILogger>             logger
        );

        explicit AnalyzeSystemUseCase(
            std::shared_ptr<abstractions::ISystemInfoService>  systemInfoService,
            std::shared_ptr<abstractions::IDiskService>        diskService,
            std::shared_ptr<abstractions::IVolumeService>      volumeService,
            std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
            std::shared_ptr<abstractions::ILogger>             logger
        );

        ~AnalyzeSystemUseCase() override = default;

        [[nodiscard]] domain::Expected<void> Execute() override;

    private:
        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SystemInfo>>
            CollectSystemInfo() const;

        [[nodiscard]] domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
            CollectDisks() const;

        [[nodiscard]] domain::Expected<std::shared_ptr<std::vector<domain::VolumeInfo>>>
            CollectVolumes() const;

        std::shared_ptr<abstractions::ISystemInfoService>  mSystemInfoService;
        std::shared_ptr<abstractions::IDiskService>        mDiskService;
        std::shared_ptr<abstractions::IVolumeService>      mVolumeService;
        std::shared_ptr<abstractions::IAnalysisRepository> mAnalysisRepository;
        std::shared_ptr<abstractions::ILogger>             mLogger;
    };

}
