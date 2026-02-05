#pragma once

#include <memory>
#include "../../../../abstractions/storage/IVolumeService.h"
#include "../../../../abstractions/platform/ITextEncoder.h"
#include <Windows.h>

namespace winsetup::adapters {

    class Win32VolumeService : public abstractions::IVolumeService {
    public:
        explicit Win32VolumeService(
            std::shared_ptr<abstractions::ITextEncoder> textEncoder
        );
        ~Win32VolumeService() override = default;

        Win32VolumeService(const Win32VolumeService&) = delete;
        Win32VolumeService& operator=(const Win32VolumeService&) = delete;
        Win32VolumeService(Win32VolumeService&&) noexcept = default;
        Win32VolumeService& operator=(Win32VolumeService&&) noexcept = default;

        [[nodiscard]] domain::Expected<std::vector<domain::VolumeInfo>>
            EnumerateVolumes() const noexcept override;

        [[nodiscard]] domain::Expected<domain::VolumeInfo>
            GetVolumeInfo(const std::wstring& volumePath) const noexcept override;

        [[nodiscard]] domain::Expected<domain::VolumeInfo>
            GetVolumeInfoByDriveLetter(wchar_t driveLetter) const noexcept override;

        [[nodiscard]] domain::Result<>
            AssignDriveLetter(
                const std::wstring& volumePath,
                wchar_t driveLetter
            ) noexcept override;

        [[nodiscard]] domain::Result<>
            RemoveDriveLetter(wchar_t driveLetter) noexcept override;

        [[nodiscard]] domain::Expected<bool>
            PathExists(const std::wstring& path) const noexcept override;

        [[nodiscard]] domain::Expected<bool>
            DirectoryExists(const std::wstring& dirPath) const noexcept override;

        [[nodiscard]] domain::Expected<bool>
            FileExists(const std::wstring& filePath) const noexcept override;

    private:
        [[nodiscard]] domain::Expected<domain::FileSystemType>
            ParseFileSystemType(const std::wstring& fsName) const noexcept;

        [[nodiscard]] domain::Expected<std::optional<wchar_t>>
            GetVolumeDriveLetter(const std::wstring& volumePath) const noexcept;

        [[nodiscard]] domain::Error
            CreateErrorFromLastError(std::string_view operation) const noexcept;

        std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
    };

    std::unique_ptr<abstractions::IVolumeService> CreateVolumeService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    );

}
