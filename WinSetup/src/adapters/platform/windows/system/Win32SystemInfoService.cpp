#include "Win32SystemInfoService.h"
#include <algorithm>

namespace winsetup::adapters {

    Win32SystemInfoService::Win32SystemInfoService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    ) noexcept
        : textEncoder_(std::move(textEncoder))
        , logger_(std::move(logger)) {
        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Win32SystemInfoService initialized");
        }
    }

    domain::Expected<abstractions::SystemInfo>
        Win32SystemInfoService::GetSystemInfo() const noexcept {
        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Retrieving system information from SMBIOS");
        }

        auto smbiosData = GetRawSMBIOSData();
        if (smbiosData.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(domain::LogLevel::Error, L"Failed to retrieve SMBIOS data");
            }
            return domain::Expected<abstractions::SystemInfo>::Failure(
                std::move(smbiosData).GetError()
            );
        }

        const auto& data = smbiosData.Value();
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"SMBIOS data size: " + std::to_wstring(data.size()) + L" bytes"
            );
        }

        if (data.size() < sizeof(SMBIOSHeader) + 8) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"SMBIOS data too small: " + std::to_wstring(data.size()) + L" bytes"
                );
            }
            return domain::Expected<abstractions::SystemInfo>::Failure(
                domain::Error("SMBIOS data too small", ERROR_INVALID_DATA)
            );
        }

        const BYTE* current = data.data() + 8;
        const BYTE* end = data.data() + data.size();

        abstractions::SystemInfo systemInfo{};
        abstractions::SystemBoardInfo baseboardInfo{};

        bool foundSystemInfo = false;
        bool foundBaseboard = false;

        if (logger_) {
            logger_->Log(domain::LogLevel::Debug, L"Parsing SMBIOS structures");
        }

        int structureCount = 0;
        while (current < end && (!foundSystemInfo || !foundBaseboard)) {
            if (current + sizeof(SMBIOSHeader) > end) [[unlikely]] {
                break;
            }

            const auto* header = reinterpret_cast<const SMBIOSHeader*>(current);
            structureCount++;

            if (header->type == 1 && !foundSystemInfo) {
                if (logger_) {
                    logger_->Log(
                        domain::LogLevel::Debug,
                        L"Found SMBIOS Type 1 (System Information) structure"
                    );
                }

                if (current + sizeof(SystemInformation) <= end) {
                    const auto* sysInfo = reinterpret_cast<const SystemInformation*>(current);
                    const size_t remainingSize = end - current;

                    systemInfo.manufacturer = GetStringAtOffset(
                        current, remainingSize, sysInfo->manufacturer
                    );
                    systemInfo.productName = GetStringAtOffset(
                        current, remainingSize, sysInfo->productName
                    );
                    systemInfo.version = GetStringAtOffset(
                        current, remainingSize, sysInfo->version
                    );

                    if (logger_) {
                        logger_->Log(
                            domain::LogLevel::Debug,
                            L"System Manufacturer: " + systemInfo.manufacturer
                        );
                        logger_->Log(
                            domain::LogLevel::Debug,
                            L"System Product: " + systemInfo.productName
                        );
                        logger_->Log(
                            domain::LogLevel::Debug,
                            L"System Version: " + systemInfo.version
                        );
                    }

                    foundSystemInfo = true;
                }
            }
            else if (header->type == 2 && !foundBaseboard) {
                if (logger_) {
                    logger_->Log(
                        domain::LogLevel::Debug,
                        L"Found SMBIOS Type 2 (Baseboard Information) structure"
                    );
                }

                if (current + sizeof(BaseboardInformation) <= end) {
                    const auto* boardInfo = reinterpret_cast<const BaseboardInformation*>(current);
                    const size_t remainingSize = end - current;

                    baseboardInfo.manufacturer = GetStringAtOffset(
                        current, remainingSize, boardInfo->manufacturer
                    );
                    baseboardInfo.product = GetStringAtOffset(
                        current, remainingSize, boardInfo->product
                    );
                    baseboardInfo.version = GetStringAtOffset(
                        current, remainingSize, boardInfo->version
                    );
                    baseboardInfo.serialNumber = GetStringAtOffset(
                        current, remainingSize, boardInfo->serialNumber
                    );

                    if (logger_) {
                        logger_->Log(
                            domain::LogLevel::Debug,
                            L"Baseboard Manufacturer: " + baseboardInfo.manufacturer
                        );
                        logger_->Log(
                            domain::LogLevel::Debug,
                            L"Baseboard Product: " + baseboardInfo.product
                        );
                        logger_->Log(
                            domain::LogLevel::Debug,
                            L"Baseboard Version: " + baseboardInfo.version
                        );
                    }

                    foundBaseboard = true;
                }
            }
            else if (header->type == 127) {
                if (logger_) {
                    logger_->Log(
                        domain::LogLevel::Debug,
                        L"Reached SMBIOS End-of-Table marker"
                    );
                }
                break;
            }

            current = FindNextStructure(current, end);
            if (!current) [[unlikely]] {
                if (logger_) {
                    logger_->Log(
                        domain::LogLevel::Warning,
                        L"SMBIOS structure parsing ended prematurely"
                    );
                }
                break;
            }
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Parsed " + std::to_wstring(structureCount) + L" SMBIOS structures"
            );
            logger_->Log(
                domain::LogLevel::Info,
                L"System information retrieved successfully"
            );
        }

        systemInfo.baseboard = baseboardInfo;

        return domain::Expected<abstractions::SystemInfo>::Success(
            std::move(systemInfo)
        );
    }

    domain::Expected<abstractions::SystemBoardInfo>
        Win32SystemInfoService::GetBaseboardInfo() const noexcept {
        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Retrieving baseboard information");
        }

        auto systemInfo = GetSystemInfo();
        if (systemInfo.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"Failed to retrieve baseboard info from system info"
                );
            }
            return domain::Expected<abstractions::SystemBoardInfo>::Failure(
                std::move(systemInfo).GetError()
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Baseboard information retrieved successfully"
            );
        }

        return domain::Expected<abstractions::SystemBoardInfo>::Success(
            std::move(systemInfo).Value().baseboard
        );
    }

    domain::Expected<std::wstring>
        Win32SystemInfoService::GetMotherboardModel() const noexcept {
        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Retrieving motherboard model");
        }

        auto baseboardInfo = GetBaseboardInfo();
        if (baseboardInfo.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(domain::LogLevel::Error, L"Failed to retrieve motherboard model");
            }
            return domain::Expected<std::wstring>::Failure(
                std::move(baseboardInfo).GetError()
            );
        }

        std::wstring model = baseboardInfo.Value().GetDisplayName();

        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Motherboard model: " + model);
        }

        return domain::Expected<std::wstring>::Success(std::move(model));
    }

    domain::Expected<std::vector<BYTE>>
        Win32SystemInfoService::GetRawSMBIOSData() const noexcept {
        if (logger_) {
            logger_->Log(domain::LogLevel::Debug, L"Querying SMBIOS firmware table size");
        }

        const DWORD signature = 'RSMB';
        DWORD bufferSize = ::GetSystemFirmwareTable(signature, 0, nullptr, 0);

        if (bufferSize == 0) [[unlikely]] {
            const DWORD error = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"GetSystemFirmwareTable size query failed with error " + std::to_wstring(error)
                );
            }
            return domain::Expected<std::vector<BYTE>>::Failure(
                CreateWindowsError("GetSystemFirmwareTable size query", error)
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"SMBIOS buffer size: " + std::to_wstring(bufferSize) + L" bytes"
            );
        }

        std::vector<BYTE> buffer(bufferSize);

        if (logger_) {
            logger_->Log(domain::LogLevel::Debug, L"Reading SMBIOS firmware table data");
        }

        const DWORD bytesWritten = ::GetSystemFirmwareTable(
            signature,
            0,
            buffer.data(),
            bufferSize
        );

        if (bytesWritten == 0 || bytesWritten != bufferSize) [[unlikely]] {
            const DWORD error = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"GetSystemFirmwareTable data read failed, expected " +
                    std::to_wstring(bufferSize) + L" bytes, got " + std::to_wstring(bytesWritten) +
                    L" bytes, error " + std::to_wstring(error)
                );
            }
            return domain::Expected<std::vector<BYTE>>::Failure(
                CreateWindowsError("GetSystemFirmwareTable data read", error)
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"SMBIOS data read successfully: " + std::to_wstring(bytesWritten) + L" bytes"
            );
        }

        return domain::Expected<std::vector<BYTE>>::Success(std::move(buffer));
    }

    std::wstring Win32SystemInfoService::GetStringAtOffset(
        const BYTE* data,
        size_t dataSize,
        BYTE offset
    ) const noexcept {

        if (offset == 0) [[unlikely]] {
            return L"";
        }

        const auto* header = reinterpret_cast<const SMBIOSHeader*>(data);
        const BYTE* stringStart = data + header->length;
        const BYTE* dataEnd = data + dataSize;

        if (stringStart >= dataEnd) [[unlikely]] {
            return L"";
        }

        BYTE currentIndex = 1;
        const BYTE* current = stringStart;

        while (current < dataEnd && currentIndex <= offset) {
            if (*current == 0) {
                if (current + 1 < dataEnd && *(current + 1) == 0) {
                    break;
                }

                if (currentIndex == offset) {
                    const char* strStart = reinterpret_cast<const char*>(stringStart);
                    std::string_view sv(strStart, current - stringStart);

                    auto wideResult = textEncoder_->ToWide(sv);
                    if (wideResult.HasError()) [[unlikely]] {
                        if (logger_) {
                            logger_->Log(
                                domain::LogLevel::Warning,
                                L"Failed to convert SMBIOS string to wide character"
                            );
                        }
                        return L"";
                    }
                    return std::move(wideResult).Value();
                }

                currentIndex++;
                stringStart = current + 1;
            }
            current++;
        }

        return L"";
    }

    const BYTE* Win32SystemInfoService::FindNextStructure(
        const BYTE* current,
        const BYTE* end
    ) const noexcept {

        if (!current || current >= end) [[unlikely]] {
            return nullptr;
        }

        const auto* header = reinterpret_cast<const SMBIOSHeader*>(current);
        const BYTE* stringArea = current + header->length;

        if (stringArea >= end) [[unlikely]] {
            return nullptr;
        }

        while (stringArea < end - 1) {
            if (stringArea[0] == 0 && stringArea[1] == 0) {
                return stringArea + 2;
            }
            stringArea++;
        }

        return nullptr;
    }

    domain::Error Win32SystemInfoService::CreateWindowsError(
        std::string_view operation,
        DWORD errorCode
    ) const noexcept {

        std::string message = "Win32SystemInfoService: ";
        message.append(operation);
        message.append(" failed with error code ");
        message.append(std::to_string(errorCode));

        return domain::Error(std::move(message), static_cast<int>(errorCode));
    }

}
