// src/abstractions/usecases/IAnalyzeSystemUseCase.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <domain/entities/DiskInfo.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::abstractions {

    struct SystemAnalysisResult {
        std::shared_ptr<const domain::SystemInfo>              systemInfo;
        std::shared_ptr<const std::vector<domain::DiskInfo>>   disks;
        std::shared_ptr<const std::vector<domain::VolumeInfo>> volumes;

        SystemAnalysisResult() = default;

        SystemAnalysisResult(
            std::shared_ptr<const domain::SystemInfo>              sysInfo,
            std::shared_ptr<const std::vector<domain::DiskInfo>>   diskList,
            std::shared_ptr<const std::vector<domain::VolumeInfo>> volumeList
        )
            : systemInfo(std::move(sysInfo))
            , disks(std::move(diskList))
            , volumes(std::move(volumeList))
        {
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return systemInfo != nullptr
                && disks != nullptr
                && volumes != nullptr;
        }
    };

    class IAnalyzeSystemUseCase {
    public:
        virtual ~IAnalyzeSystemUseCase() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<SystemAnalysisResult>>
            Execute() = 0;
    };

}
