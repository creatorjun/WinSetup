// src/adapters/platform/win32/system/Win32SystemInfoService.cpp
#include "Win32SystemInfoService.h"
#include "FirmwareTableReader.h"
#include <Windows.h>
#undef GetMessage

namespace winsetup::adapters::platform {

    Win32SystemInfoService::Win32SystemInfoService(
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mLogger(std::move(logger))
        , mSmbiosParser(nullptr)
        , mSmbiosInitialized(false)
    {
        if (mLogger) {
            mLogger->Info(L"Win32SystemInfoService initialized");
        }
    }

    domain::Expected<void> Win32SystemInfoService::EnsureSMBIOSInitialized() {
        if (mSmbiosInitialized) return domain::Expected<void>();

        if (!mSmbiosParser) {
            mSmbiosParser = std::make_unique<SMBIOSParser>();
        }

        auto initResult = mSmbiosParser->Initialize();
        if (!initResult.HasValue()) {
            if (mLogger) {
                const auto error = initResult.GetError();
                mLogger->Warning(L"Failed to initialize SMBIOS parser: "
                    + error.GetMessage());
            }
            return initResult;
        }

        mSmbiosInitialized = true;

        if (mLogger) {
            mLogger->Debug(L"SMBIOS parser initialized successfully");
        }

        return domain::Expected<void>();
    }

    domain::Expected<std::wstring> Win32SystemInfoService::GetMotherboardModel() {
        auto initResult = EnsureSMBIOSInitialized();
        if (!initResult.HasValue()) {
            if (mLogger) {
                mLogger->Warning(L"SMBIOS not available, cannot retrieve motherboard model");
            }
            return initResult.GetError();
        }

        auto modelResult = mSmbiosParser->GetMotherboardProduct();
        if (!modelResult.HasValue()) {
            if (mLogger) {
                const auto error = modelResult.GetError();
                mLogger->Warning(L"Failed to get motherboard model: " + error.GetMessage());
            }
            return modelResult.GetError();
        }

        const auto model = modelResult.Value();
        if (mLogger) {
            mLogger->Debug(L"Motherboard model: " + model);
        }

        return model;
    }

    domain::Expected<std::wstring> Win32SystemInfoService::GetBIOSVersion() {
        auto initResult = EnsureSMBIOSInitialized();
        if (!initResult.HasValue()) {
            if (mLogger) {
                mLogger->Warning(L"SMBIOS not available, cannot retrieve BIOS version");
            }
            return initResult.GetError();
        }

        auto versionResult = mSmbiosParser->GetBIOSVersion();
        if (!versionResult.HasValue()) {
            if (mLogger) {
                const auto error = versionResult.GetError();
                mLogger->Warning(L"Failed to get BIOS version: " + error.GetMessage());
            }
            return versionResult.GetError();
        }

        const auto version = versionResult.Value();
        if (mLogger) {
            mLogger->Debug(L"BIOS version: " + version);
        }

        return version;
    }

    domain::Expected<bool> Win32SystemInfoService::IsUEFIBoot() {
        FIRMWARE_TYPE firmwareType = FirmwareTypeUnknown;
        BOOL result = GetFirmwareType(&firmwareType);
        if (!result) {
            DWORD error = GetLastError();
            if (mLogger) {
                mLogger->Warning(L"Failed to get firmware type, error code: "
                    + std::to_wstring(error));
            }
            return domain::Error(
                L"Failed to determine firmware type", error, domain::ErrorCategory::System
            );
        }

        bool isUEFI = (firmwareType == FirmwareTypeUefi);
        if (mLogger) {
            mLogger->Info(isUEFI ? L"System is UEFI" : L"System is Legacy BIOS");
        }

        return isUEFI;
    }

    domain::Expected<uint64_t> Win32SystemInfoService::GetTotalMemoryBytes() {
        auto initResult = EnsureSMBIOSInitialized();
        if (initResult.HasValue()) {
            auto memoryResult = mSmbiosParser->GetTotalMemoryBytes();
            if (memoryResult.HasValue()) {
                uint64_t memory = memoryResult.Value();
                if (memory > 0) {
                    if (mLogger) {
                        mLogger->Debug(L"Total memory from SMBIOS: "
                            + std::to_wstring(memory / 1024 / 1024) + L" MB");
                    }
                    return memory;
                }
            }
        }

        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (!GlobalMemoryStatusEx(&memStatus)) {
            DWORD error = GetLastError();
            if (mLogger) {
                mLogger->Error(L"Failed to get memory status, error code: "
                    + std::to_wstring(error));
            }
            return domain::Error(
                L"Failed to get total memory", error, domain::ErrorCategory::System
            );
        }

        uint64_t totalMemory = memStatus.ullTotalPhys;
        if (mLogger) {
            mLogger->Info(L"Total physical memory: "
                + std::to_wstring(totalMemory / 1024 / 1024) + L" MB");
        }

        return totalMemory;
    }

}
