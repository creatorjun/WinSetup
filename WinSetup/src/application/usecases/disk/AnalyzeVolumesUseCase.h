// src/application/usecases/disk/AnalyzeVolumesUseCase.h
#pragma once
#include "abstractions/usecases/IAnalyzeVolumesUseCase.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/repositories/IConfigRepository.h"
#include "abstractions/services/storage/IPathChecker.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/entities/DiskInfo.h"
#include "domain/entities/VolumeInfo.h"
#include "domain/primitives/Expected.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

namespace winsetup::application {

    class AnalyzeVolumesUseCase final : public abstractions::IAnalyzeVolumesUseCase {
    public:
        explicit AnalyzeVolumesUseCase(
            std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
            std::shared_ptr<abstractions::IConfigRepository> configRepository,
            std::shared_ptr<abstractions::IPathChecker> pathChecker,
            std::shared_ptr<abstractions::ILogger> logger);

        ~AnalyzeVolumesUseCase() override = default;

        [[nodiscard]] domain::Expected<void> Execute() override;

    private:
        using DiskIndexCache = std::unordered_map<std::wstring, std::optional<uint32_t>>;

        [[nodiscard]] bool IsSystemVolume(
            const domain::VolumeInfo& volume,
            const std::wstring& userProfile) const noexcept;

        [[nodiscard]] bool IsDataVolume(
            const domain::VolumeInfo& volume,
            const std::wstring& userProfile) const noexcept;

        [[nodiscard]] bool IsBootVolume(
            const domain::VolumeInfo& volume,
            const std::vector<domain::DiskInfo>& disks,
            const DiskIndexCache& cache) const noexcept;

        void LogResult(const std::vector<domain::VolumeInfo>& volumes) const;

        std::shared_ptr<abstractions::IAnalysisRepository> mAnalysisRepository;
        std::shared_ptr<abstractions::IConfigRepository>   mConfigRepository;
        std::shared_ptr<abstractions::IPathChecker>        mPathChecker;
        std::shared_ptr<abstractions::ILogger>             mLogger;
    };

} // namespace winsetup::application
