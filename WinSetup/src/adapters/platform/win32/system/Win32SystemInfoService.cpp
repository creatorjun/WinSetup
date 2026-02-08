// src/adapters/platform/win32/system/Win32SystemInfoService.cpp
#include "Win32SystemInfoService.h"

namespace winsetup::adapters::platform {

    Win32SystemInfoService::Win32SystemInfoService(
        std::shared_ptr<abstractions::ILogger> logger)
        : m_logger(std::move(logger))
    {
        if (m_logger) {
            m_logger->Info(L"Win32SystemInfoService initialized");
        }
    }

    domain::Expected<std::wstring> Win32SystemInfoService::GetMotherboardModel() {
        return std::wstring(L"Unknown Motherboard");
    }

    domain::Expected<std::wstring> Win32SystemInfoService::GetBIOSVersion() {
        return std::wstring(L"Unknown BIOS");
    }

    domain::Expected<bool> Win32SystemInfoService::IsUEFIBoot() {
        return false;
    }

    domain::Expected<uint64_t> Win32SystemInfoService::GetTotalMemoryBytes() {
        return static_cast<uint64_t>(8ULL * 1024 * 1024 * 1024);
    }

}
