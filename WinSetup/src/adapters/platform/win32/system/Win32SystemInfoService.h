// src/adapters/platform/win32/system/Win32SystemInfoService.h
#pragma once

#include <abstractions/services/platform/ISystemInfoService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include "SMBIOSParser.h"
#include <memory>

namespace winsetup::adapters::platform {

    class Win32SystemInfoService : public abstractions::ISystemInfoService {
    public:
        explicit Win32SystemInfoService(
            std::shared_ptr<abstractions::ILogger> logger);
        ~Win32SystemInfoService() override = default;

        [[nodiscard]] domain::Expected<std::wstring>
            GetMotherboardModel() override;

        [[nodiscard]] domain::Expected<std::wstring>
            GetBIOSVersion() override;

        [[nodiscard]] domain::Expected<bool>
            IsUEFIBoot() override;

        [[nodiscard]] domain::Expected<uint64_t>
            GetTotalMemoryBytes() override;

    private:
        [[nodiscard]] domain::Expected<void> EnsureSMBIOSInitialized();

        std::shared_ptr<abstractions::ILogger> mLogger;
        std::unique_ptr<SMBIOSParser> mSmbiosParser;
        bool mSmbiosInitialized;
    };

}
