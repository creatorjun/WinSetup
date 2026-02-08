// src/adapters/platform/win32/storage/Win32VolumeService.h
#pragma once

#include <abstractions/services/storage/IVolumeService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <memory>

namespace winsetup::adapters::platform {

    class Win32VolumeService : public abstractions::IVolumeService {
    public:
        explicit Win32VolumeService(std::shared_ptr<abstractions::ILogger> logger);
        ~Win32VolumeService() override = default;

        [[nodiscard]] domain::Expected<std::vector<domain::VolumeInfo>>
            EnumerateVolumes() override;

        [[nodiscard]] domain::Expected<domain::VolumeInfo>
            GetVolumeInfo(const std::wstring& volumePath) override;

        [[nodiscard]] domain::Expected<void>
            MountVolume(const std::wstring& volumePath, wchar_t driveLetter) override;

        [[nodiscard]] domain::Expected<void>
            DismountVolume(wchar_t driveLetter) override;

    private:
        std::shared_ptr<abstractions::ILogger> m_logger;
    };

}
