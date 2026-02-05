#pragma once

#include <memory>
#include "../../../../abstractions/storage/IStorageScanner.h"
#include "../../../../abstractions/storage/IVolumeService.h"
#include "../../../../abstractions/platform/ITextEncoder.h"

namespace winsetup::adapters {

    class Win32StorageScanner : public abstractions::IStorageScanner {
    public:
        Win32StorageScanner(
            std::shared_ptr<abstractions::IVolumeService> volumeService,
            std::shared_ptr<abstractions::ITextEncoder> textEncoder
        );
        ~Win32StorageScanner() override = default;

        Win32StorageScanner(const Win32StorageScanner&) = delete;
        Win32StorageScanner& operator=(const Win32StorageScanner&) = delete;
        Win32StorageScanner(Win32StorageScanner&&) noexcept = default;
        Win32StorageScanner& operator=(Win32StorageScanner&&) noexcept = default;

        [[nodiscard]] domain::Expected<abstractions::ScanResult>
            ScanVolume(
                const domain::VolumeInfo& volume,
                const std::wstring& userProfile
            ) const noexcept override;

        [[nodiscard]] domain::Expected<std::vector<std::wstring>>
            FindUserDataDirectories(
                const domain::VolumeInfo& volume,
                const std::wstring& userProfile
            ) const noexcept override;

    private:
        [[nodiscard]] int CalculateSystemScore(
            bool hasWindows,
            bool hasSystem32,
            bool hasUsers,
            bool hasProgramFiles
        ) const noexcept;

        [[nodiscard]] int CalculateDataScore(
            const std::vector<std::wstring>& userDirs
        ) const noexcept;

        std::shared_ptr<abstractions::IVolumeService> volumeService_;
        std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
    };

    std::unique_ptr<abstractions::IStorageScanner> CreateStorageScanner(
        std::shared_ptr<abstractions::IVolumeService> volumeService,
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    );

}
