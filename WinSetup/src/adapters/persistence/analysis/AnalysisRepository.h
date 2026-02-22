// src/adapters/persistence/analysis/AnalysisRepository.h
#pragma once

#include <abstractions/repositories/IAnalysisRepository.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <domain/entities/DiskInfo.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::adapters::persistence {

    class AnalysisRepository : public abstractions::IAnalysisRepository {
    public:
        AnalysisRepository() = default;
        ~AnalysisRepository() override = default;

        void StoreSystemInfo(
            std::shared_ptr<const domain::SystemInfo> systemInfo
        ) override;

        void StoreDisks(
            std::shared_ptr<const std::vector<domain::DiskInfo>> disks
        ) override;

        void StoreVolumes(
            std::shared_ptr<const std::vector<domain::VolumeInfo>> volumes
        ) override;

        [[nodiscard]] domain::Expected<std::shared_ptr<const domain::SystemInfo>>
            GetSystemInfo() const override;

        [[nodiscard]] domain::Expected<std::shared_ptr<const std::vector<domain::DiskInfo>>>
            GetDisks() const override;

        [[nodiscard]] domain::Expected<std::shared_ptr<const std::vector<domain::VolumeInfo>>>
            GetVolumes() const override;

        [[nodiscard]] bool IsLoaded() const noexcept override;

        void Clear() noexcept override;

    private:
        std::shared_ptr<const domain::SystemInfo>              mSystemInfo;
        std::shared_ptr<const std::vector<domain::DiskInfo>>   mDisks;
        std::shared_ptr<const std::vector<domain::VolumeInfo>> mVolumes;
    };

}
