// src/adapters/platform/win32/system/Win32SystemInfoService.cpp
#include "Win32SystemInfoService.h"
#include "FirmwareTableReader.h"
#include <Windows.h>
#undef GetMessage

namespace winsetup::adapters::platform {

    Win32SystemInfoService::Win32SystemInfoService(std::shared_ptr<abstractions::ILogger> logger)
        : m_logger(std::move(logger))
        , m_smbiosParser(nullptr)
        , m_smbiosInitialized(false)
    {
        if (m_logger) m_logger->Info(L"Win32SystemInfoService initialized");
    }

    domain::Expected<void> Win32SystemInfoService::EnsureSMBIOSInitialized() {
        if (m_smbiosInitialized) return domain::Expected<void>();
        if (!m_smbiosParser) m_smbiosParser = std::make_unique<SMBIOSParser>();

        auto initResult = m_smbiosParser->Initialize();
        if (!initResult.HasValue()) {
            if (m_logger) {
                const auto& error = initResult.GetError();
                m_logger->Warning(L"Failed to initialize SMBIOS parser: " + error.GetMessage());
            }
            return initResult;
        }

        m_smbiosInitialized = true;
        if (m_logger) m_logger->Debug(L"SMBIOS parser initialized successfully");
        return domain::Expected<void>();
    }

    domain::Expected<std::wstring> Win32SystemInfoService::GetMotherboardModel() {
        auto initResult = EnsureSMBIOSInitialized();
        if (!initResult.HasValue()) {
            if (m_logger) m_logger->Warning(L"SMBIOS not available, cannot retrieve motherboard model");
            return initResult.GetError();
        }

        auto modelResult = m_smbiosParser->GetMotherboardModel();
        if (!modelResult.HasValue()) {
            if (m_logger) {
                const auto& error = modelResult.GetError();
                m_logger->Warning(L"Failed to get motherboard model: " + error.GetMessage());
            }
            return modelResult.GetError();
        }

        const auto& model = modelResult.Value();
        if (m_logger) m_logger->Debug(L"Motherboard model: " + model);
        return model;
    }

    domain::Expected<std::wstring> Win32SystemInfoService::GetBIOSVersion() {
        auto initResult = EnsureSMBIOSInitialized();
        if (!initResult.HasValue()) {
            if (m_logger) m_logger->Warning(L"SMBIOS not available, cannot retrieve BIOS version");
            return initResult.GetError();
        }

        auto versionResult = m_smbiosParser->GetBIOSVersion();
        if (!versionResult.HasValue()) {
            if (m_logger) {
                const auto& error = versionResult.GetError();
                m_logger->Warning(L"Failed to get BIOS version: " + error.GetMessage());
            }
            return versionResult.GetError();
        }

        const auto& version = versionResult.Value();
        if (m_logger) m_logger->Debug(L"BIOS version: " + version);
        return version;
    }

    domain::Expected<bool> Win32SystemInfoService::IsUEFIBoot() {
        FIRMWARE_TYPE firmwareType;
        DWORD dataSize = sizeof(firmwareType);
        BOOL result = GetFirmwareType(&firmwareType);

        if (!result) {
            DWORD error = GetLastError();
            if (m_logger)
                m_logger->Warning(L"Failed to get firmware type, error code: " + std::to_wstring(error));
            return domain::Error(L"Failed to determine firmware type", error, domain::ErrorCategory::System);
        }

        bool isUEFI = (firmwareType == FirmwareTypeUefi);
        if (m_logger) m_logger->Info(isUEFI ? L"System is UEFI" : L"System is Legacy BIOS");
        return isUEFI;
    }

    domain::Expected<uint64_t> Win32SystemInfoService::GetTotalMemoryBytes() {
        auto initResult = EnsureSMBIOSInitialized();
        if (initResult.HasValue()) {
            auto memoryResult = m_smbiosParser->GetTotalMemoryBytes();
            if (memoryResult.HasValue()) {
                uint64_t memory = memoryResult.Value();
                if (memory > 0) {
                    if (m_logger)
                        m_logger->Debug(L"Total memory from SMBIOS: " +
                            std::to_wstring(memory / 1024 / 1024) + L" MB");
                    return memory;
                }
            }
        }

        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (!GlobalMemoryStatusEx(&memStatus)) {
            DWORD error = GetLastError();
            if (m_logger)
                m_logger->Error(L"Failed to get memory status, error code: " + std::to_wstring(error));
            return domain::Error(L"Failed to get total memory", error, domain::ErrorCategory::System);
        }

        uint64_t totalMemory = memStatus.ullTotalPhys;
        if (m_logger)
            m_logger->Info(L"Total physical memory: " +
                std::to_wstring(totalMemory / 1024 / 1024) + L" MB");
        return totalMemory;
    }

}
