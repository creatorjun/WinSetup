// src/adapters/platform/win32/system/SMBIOSParser.h
#pragma once

#include <domain/primitives/Expected.h>
#include <cstdint>
#include <string>
#include <vector>

namespace winsetup::adapters::platform {

    struct SMBIOSBIOSInfo {
        std::wstring vendor;
        std::wstring version;
        std::wstring releaseDate;
    };

    struct SMBIOSSystemInfo {
        std::wstring manufacturer;
        std::wstring productName;
        std::wstring version;
        std::wstring serialNumber;
        uint8_t uuid[16];
    };

    struct SMBIOSBaseboardInfo {
        std::wstring manufacturer;
        std::wstring product;
        std::wstring version;
        std::wstring serialNumber;
    };

    struct SMBIOSProcessorInfo {
        std::wstring manufacturer;
        std::wstring version;
        uint16_t maxSpeed;
        uint16_t currentSpeed;
        uint8_t coreCount;
        uint8_t threadCount;
    };

    struct SMBIOSMemoryDeviceInfo {
        std::wstring manufacturer;
        std::wstring partNumber;
        std::wstring serialNumber;
        uint64_t sizeBytes;
        uint16_t speed;
        uint16_t type;
    };

    struct SMBIOSInfo {
        SMBIOSBIOSInfo biosInfo;
        SMBIOSSystemInfo systemInfo;
        SMBIOSBaseboardInfo baseboardInfo;
        SMBIOSProcessorInfo processorInfo;
        std::vector<SMBIOSMemoryDeviceInfo> memoryDevices;
    };

    class SMBIOSParser {
    public:
        SMBIOSParser() = default;
        ~SMBIOSParser() = default;

        SMBIOSParser(const SMBIOSParser&) = delete;
        SMBIOSParser& operator=(const SMBIOSParser&) = delete;

        [[nodiscard]] domain::Expected<void>        Initialize();
        [[nodiscard]] domain::Expected<SMBIOSInfo>  GetSystemInfo();
        [[nodiscard]] domain::Expected<std::wstring> GetBIOSVersion();
        [[nodiscard]] domain::Expected<std::wstring> GetBIOSVendor();
        [[nodiscard]] domain::Expected<std::wstring> GetMotherboardManufacturer();
        [[nodiscard]] domain::Expected<std::wstring> GetMotherboardProduct();
        [[nodiscard]] domain::Expected<std::wstring> GetMotherboardModel();
        [[nodiscard]] domain::Expected<std::wstring> GetSystemManufacturer();
        [[nodiscard]] domain::Expected<std::wstring> GetSystemProductName();
        [[nodiscard]] domain::Expected<uint64_t>     GetTotalMemoryBytes();
        [[nodiscard]] bool IsParsed() const noexcept { return mParsed; }

    private:
        [[nodiscard]] domain::Expected<void> ParseTables();
        void ParseBIOSInformation(const uint8_t* data);
        void ParseSystemInformation(const uint8_t* data);
        void ParseBaseboardInformation(const uint8_t* data);
        void ParseProcessorInformation(const uint8_t* data);
        void ParseMemoryDevice(const uint8_t* data);

        std::vector<uint8_t> mRawData;
        bool                 mParsed = false;

        SMBIOSBIOSInfo                   mBIOSInfo;
        SMBIOSSystemInfo                 mSystemInfo;
        SMBIOSBaseboardInfo              mBaseboardInfo;
        SMBIOSProcessorInfo              mProcessorInfo;
        std::vector<SMBIOSMemoryDeviceInfo> mMemoryDevices;
    };

}
