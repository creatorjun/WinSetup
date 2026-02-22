// src/abstractions/repositories/IAnalysisRepository.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <domain/entities/DiskInfo.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::abstractions {

    class IAnalysisRepository {
    public:
        virtual ~IAnalysisRepository() = default;

        virtual void StoreSystemInfo(
            std::shared_ptr<const domain::SystemInfo> systemInfo
        ) = 0;

        virtual void StoreDisks(
            std::shared_ptr<const std::vector<domain::DiskInfo>> disks
        ) = 0;

        virtual void StoreVolumes(
            std::shared_ptr<const std::vector<domain::VolumeInfo>> volumes
        ) = 0;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<const domain::SystemInfo>>
            GetSystemInfo() const = 0;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<const std::vector<domain::DiskInfo>>>
            GetDisks() const = 0;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<const std::vector<domain::VolumeInfo>>>
            GetVolumes() const = 0;

        [[nodiscard]] virtual bool IsLoaded() const noexcept = 0;

        virtual void Clear() noexcept = 0;
    };

}
