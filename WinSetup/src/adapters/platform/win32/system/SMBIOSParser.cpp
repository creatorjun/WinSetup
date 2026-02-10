// src/adapters/platform/win32/system/SMBIOSParser.cpp
#include "SMBIOSParser.h"
#include "SMBIOSStructures.h"
#include "../core/Win32ErrorHandler.h"
#include <algorithm>
#include <cstring>
#include <cwctype>

namespace winsetup::adapters::platform {

    namespace {
        const char* GetStringFromTable(const BYTE* tableStart, BYTE stringIndex) {
            if (stringIndex == 0) {
                return "";
            }

            const BYTE* ptr = tableStart;

            ptr += ptr[1];

            BYTE currentIndex = 1;
            while (*ptr != 0 || *(ptr + 1) != 0) {
                if (currentIndex == stringIndex) {
                    return reinterpret_cast<const char*>(ptr);
                }

                while (*ptr != 0) {
                    ptr++;
                }
                ptr++;
                currentIndex++;
            }

            return "";
        }

        std::wstring ConvertToWString(const char* str) {
            if (!str || *str == '\0') {
                return L"";
            }

            int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
            if (len <= 0) {
                return L"";
            }

            std::wstring result(len - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, str, -1, &result[0], len);

            return result;
        }

        std::wstring TrimString(std::wstring str) {
            str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](wchar_t ch) {
                return !iswspace(static_cast<wint_t>(ch));
                }));

            str.erase(std::find_if(str.rbegin(), str.rend(), [](wchar_t ch) {
                return !iswspace(static_cast<wint_t>(ch));
                }).base(), str.end());

            return str;
        }
    }

    SMBIOSParser::SMBIOSParser()
        : mRawData(nullptr)
        , mDataSize(0)
        , mParsed(false)
    {
    }

    SMBIOSParser::~SMBIOSParser() {
        if (mRawData) {
            delete[] mRawData;
            mRawData = nullptr;
        }
    }

    domain::Expected<void> SMBIOSParser::Initialize() {
        DWORD bufferSize = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
        if (bufferSize == 0) {
            return domain::Error{
                L"Failed to get SMBIOS table size",
                GetLastError(),
                domain::ErrorCategory::System
            };
        }

        mRawData = new (std::nothrow) BYTE[bufferSize];
        if (!mRawData) {
            return domain::Error{
                L"Failed to allocate memory for SMBIOS table",
                ERROR_OUTOFMEMORY,
                domain::ErrorCategory::System
            };
        }

        DWORD result = GetSystemFirmwareTable('RSMB', 0, mRawData, bufferSize);
        if (result == 0) {
            delete[] mRawData;
            mRawData = nullptr;
            return domain::Error{
                L"Failed to read SMBIOS table",
                GetLastError(),
                domain::ErrorCategory::System
            };
        }

        mDataSize = bufferSize;

        auto parseResult = ParseTables();
        if (!parseResult.HasValue()) {
            return parseResult;
        }

        mParsed = true;
        return domain::Expected<void>();
    }

    domain::Expected<SMBIOSInfo> SMBIOSParser::GetSystemInfo() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        SMBIOSInfo info{};

        info.biosInfo = mBIOSInfo;
        info.systemInfo = mSystemInfo;
        info.baseboardInfo = mBaseboardInfo;
        info.processorInfo = mProcessorInfo;
        info.memoryDevices = mMemoryDevices;

        return info;
    }

    domain::Expected<std::wstring> SMBIOSParser::GetBIOSVersion() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        if (mBIOSInfo.version.empty()) {
            return std::wstring(L"Unknown");
        }

        return mBIOSInfo.version;
    }

    domain::Expected<std::wstring> SMBIOSParser::GetBIOSVendor() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        if (mBIOSInfo.vendor.empty()) {
            return std::wstring(L"Unknown");
        }

        return mBIOSInfo.vendor;
    }

    domain::Expected<std::wstring> SMBIOSParser::GetMotherboardManufacturer() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        if (mBaseboardInfo.manufacturer.empty()) {
            return std::wstring(L"Unknown");
        }

        return mBaseboardInfo.manufacturer;
    }

    domain::Expected<std::wstring> SMBIOSParser::GetMotherboardProduct() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        if (mBaseboardInfo.product.empty()) {
            return std::wstring(L"Unknown");
        }

        return mBaseboardInfo.product;
    }

    domain::Expected<std::wstring> SMBIOSParser::GetMotherboardModel() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        std::wstring model;

        if (!mBaseboardInfo.manufacturer.empty()) {
            model = mBaseboardInfo.manufacturer;
        }

        if (!mBaseboardInfo.product.empty()) {
            if (!model.empty()) {
                model += L" ";
            }
            model += mBaseboardInfo.product;
        }

        if (model.empty()) {
            return std::wstring(L"Unknown Motherboard");
        }

        return TrimString(model);
    }

    domain::Expected<std::wstring> SMBIOSParser::GetSystemManufacturer() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        if (mSystemInfo.manufacturer.empty()) {
            return std::wstring(L"Unknown");
        }

        return mSystemInfo.manufacturer;
    }

    domain::Expected<std::wstring> SMBIOSParser::GetSystemProductName() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        if (mSystemInfo.productName.empty()) {
            return std::wstring(L"Unknown");
        }

        return mSystemInfo.productName;
    }

    domain::Expected<uint64_t> SMBIOSParser::GetTotalMemoryBytes() {
        if (!mParsed) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult.GetError();
            }
        }

        uint64_t totalMemory = 0;

        for (const auto& memDevice : mMemoryDevices) {
            totalMemory += memDevice.sizeBytes;
        }

        return totalMemory;
    }

    domain::Expected<void> SMBIOSParser::ParseTables() {
        if (!mRawData || mDataSize < sizeof(RawSMBIOSData)) {
            return domain::Error{
                L"Invalid SMBIOS data",
                ERROR_INVALID_DATA,
                domain::ErrorCategory::System
            };
        }

        const RawSMBIOSData* rawTable = reinterpret_cast<const RawSMBIOSData*>(mRawData);

        const BYTE* tableData = rawTable->SMBIOSTableData;
        const BYTE* tableEnd = mRawData + mDataSize;
        const BYTE* currentPtr = tableData;

        while (currentPtr + sizeof(SMBIOSHeader) < tableEnd) {
            const SMBIOSHeader* header = reinterpret_cast<const SMBIOSHeader*>(currentPtr);

            if (header->Length < sizeof(SMBIOSHeader)) {
                break;
            }

            if (currentPtr + header->Length > tableEnd) {
                break;
            }

            switch (header->Type) {
            case 0:
                ParseBIOSInformation(currentPtr);
                break;
            case 1:
                ParseSystemInformation(currentPtr);
                break;
            case 2:
                ParseBaseboardInformation(currentPtr);
                break;
            case 4:
                ParseProcessorInformation(currentPtr);
                break;
            case 17:
                ParseMemoryDevice(currentPtr);
                break;
            default:
                break;
            }

            currentPtr += header->Length;

            while (currentPtr + 1 < tableEnd) {
                if (*currentPtr == 0 && *(currentPtr + 1) == 0) {
                    currentPtr += 2;
                    break;
                }
                currentPtr++;
            }

            if (header->Type == 127) {
                break;
            }
        }

        return domain::Expected<void>();
    }

    void SMBIOSParser::ParseBIOSInformation(const BYTE* data) {
        const auto* bios = reinterpret_cast<const SMBIOSBIOSInformation*>(data);

        mBIOSInfo.vendor = ConvertToWString(GetStringFromTable(data, bios->Vendor));
        mBIOSInfo.version = ConvertToWString(GetStringFromTable(data, bios->BIOSVersion));
        mBIOSInfo.releaseDate = ConvertToWString(GetStringFromTable(data, bios->BIOSReleaseDate));
    }

    void SMBIOSParser::ParseSystemInformation(const BYTE* data) {
        const auto* system = reinterpret_cast<const SMBIOSSystemInformation*>(data);

        mSystemInfo.manufacturer = ConvertToWString(GetStringFromTable(data, system->Manufacturer));
        mSystemInfo.productName = ConvertToWString(GetStringFromTable(data, system->ProductName));
        mSystemInfo.version = ConvertToWString(GetStringFromTable(data, system->Version));
        mSystemInfo.serialNumber = ConvertToWString(GetStringFromTable(data, system->SerialNumber));

        std::memcpy(mSystemInfo.uuid, system->UUID, 16);
    }

    void SMBIOSParser::ParseBaseboardInformation(const BYTE* data) {
        const auto* baseboard = reinterpret_cast<const SMBIOSBaseboardInformation*>(data);

        mBaseboardInfo.manufacturer = ConvertToWString(GetStringFromTable(data, baseboard->Manufacturer));
        mBaseboardInfo.product = ConvertToWString(GetStringFromTable(data, baseboard->Product));
        mBaseboardInfo.version = ConvertToWString(GetStringFromTable(data, baseboard->Version));
        mBaseboardInfo.serialNumber = ConvertToWString(GetStringFromTable(data, baseboard->SerialNumber));
    }

    void SMBIOSParser::ParseProcessorInformation(const BYTE* data) {
        const auto* processor = reinterpret_cast<const SMBIOSProcessorInformation*>(data);

        SMBIOSProcessorInfo info{};
        info.manufacturer = ConvertToWString(GetStringFromTable(data, processor->ProcessorManufacturer));
        info.version = ConvertToWString(GetStringFromTable(data, processor->ProcessorVersion));
        info.maxSpeed = processor->MaxSpeed;
        info.currentSpeed = processor->CurrentSpeed;
        info.coreCount = processor->CoreCount;
        info.threadCount = processor->ThreadCount;

        mProcessorInfo = info;
    }

    void SMBIOSParser::ParseMemoryDevice(const BYTE* data) {
        const auto* memory = reinterpret_cast<const SMBIOSMemoryDevice*>(data);

        if (memory->Size == 0 || memory->Size == 0xFFFF) {
            return;
        }

        SMBIOSMemoryDeviceInfo info{};
        info.manufacturer = ConvertToWString(GetStringFromTable(data, memory->Manufacturer));
        info.partNumber = ConvertToWString(GetStringFromTable(data, memory->PartNumber));
        info.serialNumber = ConvertToWString(GetStringFromTable(data, memory->SerialNumber));

        if (memory->Size == 0x7FFF) {
            info.sizeBytes = static_cast<uint64_t>(memory->ExtendedSize) * 1024 * 1024;
        }
        else {
            uint64_t sizeMB = memory->Size;
            if (!(memory->Size & 0x8000)) {
                sizeMB *= 1024;
            }
            else {
                sizeMB = (memory->Size & 0x7FFF);
            }
            info.sizeBytes = sizeMB * 1024 * 1024;
        }

        info.speed = memory->Speed;
        info.type = memory->MemoryType;

        mMemoryDevices.push_back(info);
    }

}
