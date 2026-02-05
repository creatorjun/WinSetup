#pragma once

#include <vector>
#include <string>
#include "../../domain/primitives/Expected.h"
#include "../../domain/entities/VolumeInfo.h"

namespace winsetup::abstractions {

    struct ScanResult {
        bool hasWindowsDirectory{ false };
        bool hasSystem32Directory{ false };
        bool hasUsersDirectory{ false };
        bool hasProgramFilesDirectory{ false };
        int score{ 0 };

        [[nodiscard]] bool IsLikelySystemVolume() const noexcept {
            return hasWindowsDirectory && hasSystem32Directory && score >= 80;
        }

        [[nodiscard]] bool IsLikelyDataVolume() const noexcept {
            return hasUsersDirectory && !hasSystem32Directory && score >= 50;
        }
    };

    class IStorageScanner {
    public:
        virtual ~IStorageScanner() = default;

        [[nodiscard]] virtual domain::Expected<ScanResult>
            ScanVolume(
                const domain::VolumeInfo& volume,
                const std::wstring& userProfile
            ) const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<std::vector<std::wstring>>
            FindUserDataDirectories(
                const domain::VolumeInfo& volume,
                const std::wstring& userProfile
            ) const noexcept = 0;
    };

}
