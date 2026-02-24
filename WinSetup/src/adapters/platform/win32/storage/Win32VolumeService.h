#pragma once
#include "abstractions/services/storage/IVolumeService.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "adapters/platform/win32/memory/UniqueHandle.h"
#include <memory>
#include <Windows.h>

namespace winsetup::adapters::platform {

    class Win32VolumeService final : public abstractions::IVolumeService {
    public:
        explicit Win32VolumeService(std::shared_ptr<abstractions::ILogger> logger);
        ~Win32VolumeService() override = default;

        Win32VolumeService(const Win32VolumeService&) = delete;
        Win32VolumeService& operator=(const Win32VolumeService&) = delete;

        [[nodiscard]] domain::Expected<std::vector<domain::VolumeInfo>> EnumerateVolumes() override;
        [[nodiscard]] domain::Expected<domain::VolumeInfo> GetVolumeInfo(const std::wstring& volumePath) override;
        [[nodiscard]] domain::Expected<void> MountVolume(const std::wstring& volumeGuid, wchar_t driveLetter) override;
        [[nodiscard]] domain::Expected<void> DismountVolume(wchar_t driveLetter) override;

    private:
        [[nodiscard]] domain::Expected<std::wstring> GetVolumeLabel(const std::wstring& volumePath);
        [[nodiscard]] domain::Expected<domain::FileSystemType> GetFileSystem(const std::wstring& volumePath);
        [[nodiscard]] domain::Expected<domain::DiskSize> GetVolumeSize(const std::wstring& volumePath);
        [[nodiscard]] domain::Expected<std::wstring> GetVolumeType(const std::wstring& volumePath);
        [[nodiscard]] domain::Expected<std::vector<std::wstring>> GetDriveLetters(const std::wstring& volumeGuid);
        [[nodiscard]] bool IsVolumeMounted(const std::wstring& volumePath);

        std::shared_ptr<abstractions::ILogger> mLogger;
    };

} // namespace winsetup::adapters::platform
