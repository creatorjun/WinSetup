#pragma once

#include <vector>
#include <optional>
#include "../../domain/primitives/Expected.h"
#include "../../domain/primitives/Result.h"
#include "../../domain/entities/VolumeInfo.h"
#include "../../domain/entities/PartitionInfo.h"

namespace winsetup::abstractions {

    class IVolumeService {
    public:
        virtual ~IVolumeService() = default;

        [[nodiscard]] virtual domain::Expected<std::vector<domain::VolumeInfo>>
            EnumerateVolumes() const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<domain::VolumeInfo>
            GetVolumeInfo(const std::wstring& volumePath) const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<domain::VolumeInfo>
            GetVolumeInfoByDriveLetter(wchar_t driveLetter) const noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            AssignDriveLetter(
                const std::wstring& volumePath,
                wchar_t driveLetter
            ) noexcept = 0;

        [[nodiscard]] virtual domain::Result<>
            RemoveDriveLetter(wchar_t driveLetter) noexcept = 0;

        [[nodiscard]] virtual domain::Expected<bool>
            PathExists(const std::wstring& path) const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<bool>
            DirectoryExists(const std::wstring& dirPath) const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<bool>
            FileExists(const std::wstring& filePath) const noexcept = 0;
    };

}
