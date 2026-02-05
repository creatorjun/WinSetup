#pragma once

#include <vector>
#include <memory>
#include <Windows.h>
#include <abstractions/logging/ILogger.h>
#include <abstractions/logging/LogLevel.h>
#include <abstractions/platform/ITextEncoder.h>
#include <abstractions/platform/ISystemInfoService.h>

namespace winsetup::adapters {

    class Win32SystemInfoService : public abstractions::ISystemInfoService {
    public:
        explicit Win32SystemInfoService(
            std::shared_ptr<abstractions::ITextEncoder> textEncoder,
            std::shared_ptr<abstractions::ILogger> logger
        ) noexcept;

        ~Win32SystemInfoService() override = default;

        Win32SystemInfoService(const Win32SystemInfoService&) = delete;
        Win32SystemInfoService& operator=(const Win32SystemInfoService&) = delete;
        Win32SystemInfoService(Win32SystemInfoService&&) noexcept = default;
        Win32SystemInfoService& operator=(Win32SystemInfoService&&) noexcept = default;

        [[nodiscard]] domain::Expected<abstractions::SystemInfo>
            GetSystemInfo() const noexcept override;

        [[nodiscard]] domain::Expected<abstractions::SystemBoardInfo>
            GetBaseboardInfo() const noexcept override;

        [[nodiscard]] domain::Expected<std::wstring>
            GetMotherboardModel() const noexcept override;

    private:
        std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
        std::shared_ptr<abstractions::ILogger> logger_;

        struct SMBIOSHeader {
            BYTE type;
            BYTE length;
            WORD handle;
        };

        struct SystemInformation {
            SMBIOSHeader header;
            BYTE manufacturer;
            BYTE productName;
            BYTE version;
            BYTE serialNumber;
        };

        struct BaseboardInformation {
            SMBIOSHeader header;
            BYTE manufacturer;
            BYTE product;
            BYTE version;
            BYTE serialNumber;
        };

        [[nodiscard]] domain::Expected<std::vector<BYTE>>
            GetRawSMBIOSData() const noexcept;

        [[nodiscard]] std::wstring GetStringAtOffset(
            const BYTE* data,
            size_t dataSize,
            BYTE offset
        ) const noexcept;

        [[nodiscard]] const BYTE* FindNextStructure(
            const BYTE* current,
            const BYTE* end
        ) const noexcept;

        [[nodiscard]] domain::Error CreateWindowsError(
            std::string_view operation,
            DWORD errorCode
        ) const noexcept;
    };

}
