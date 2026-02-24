#pragma once
#include "domain/primitives/Expected.h"
#include "domain/entities/VolumeInfo.h"
#include <vector>
#include <cstdint>

namespace winsetup::abstractions {

    class IVolumeService {
    public:
        virtual ~IVolumeService() = default;

        [[nodiscard]] virtual domain::Expected<std::vector<domain::VolumeInfo>> EnumerateVolumes() = 0;
        [[nodiscard]] virtual domain::Expected<domain::VolumeInfo> GetVolumeInfo(const std::wstring& volumePath) = 0;
        [[nodiscard]] virtual domain::Expected<void> MountVolume(const std::wstring& volumeGuid, wchar_t driveLetter) = 0;
        [[nodiscard]] virtual domain::Expected<void> DismountVolume(wchar_t driveLetter) = 0;
    };

} // namespace winsetup::abstractions
