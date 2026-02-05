#include "Win32SystemInfoService.h"
#include <algorithm>

namespace winsetup::adapters {

    Win32SystemInfoService::Win32SystemInfoService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    ) noexcept
        : textEncoder_(std::move(textEncoder)) {
    }

    domain::Expected<abstractions::SystemInfo>
        Win32SystemInfoService::GetSystemInfo() const noexcept {

        auto smbiosData = GetRawSMBIOSData();
        if (smbiosData.HasError()) [[unlikely]] {
            return domain::Expected<abstractions::SystemInfo>::Failure(
                std::move(smbiosData).GetError()
            );
        }

        const auto& data = smbiosData.Value();
        if (data.size() < sizeof(SMBIOSHeader) + 8) [[unlikely]] {
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

        while (current < end && (!foundSystemInfo || !foundBaseboard)) {
            if (current + sizeof(SMBIOSHeader) > end) [[unlikely]] {
                break;
            }

            const auto* header = reinterpret_cast<const SMBIOSHeader*>(current);

            if (header->type == 1 && !foundSystemInfo) {
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

                    foundSystemInfo = true;
                }
            }
            else if (header->type == 2 && !foundBaseboard) {
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

                    foundBaseboard = true;
                }
            }
            else if (header->type == 127) {
                break;
            }

            current = FindNextStructure(current, end);
            if (!current) [[unlikely]] {
                break;
            }
        }

        systemInfo.baseboard = baseboardInfo;

        return domain::Expected<abstractions::SystemInfo>::Success(
            std::move(systemInfo)
        );
    }

    domain::Expected<abstractions::SystemBoardInfo>
        Win32SystemInfoService::GetBaseboardInfo() const noexcept {

        auto systemInfo = GetSystemInfo();
        if (systemInfo.HasError()) [[unlikely]] {
            return domain::Expected<abstractions::SystemBoardInfo>::Failure(
                std::move(systemInfo).GetError()
            );
        }

        return domain::Expected<abstractions::SystemBoardInfo>::Success(
            std::move(systemInfo).Value().baseboard
        );
    }

    domain::Expected<std::wstring>
        Win32SystemInfoService::GetMotherboardModel() const noexcept {

        auto baseboardInfo = GetBaseboardInfo();
        if (baseboardInfo.HasError()) [[unlikely]] {
            return domain::Expected<std::wstring>::Failure(
                std::move(baseboardInfo).GetError()
            );
        }

        return domain::Expected<std::wstring>::Success(
            baseboardInfo.Value().GetDisplayName()
        );
    }

    domain::Expected<std::vector<BYTE>>
        Win32SystemInfoService::GetRawSMBIOSData() const noexcept {

        const DWORD signature = 'RSMB';
        DWORD bufferSize = ::GetSystemFirmwareTable(signature, 0, nullptr, 0);

        if (bufferSize == 0) [[unlikely]] {
            const DWORD error = ::GetLastError();
            return domain::Expected<std::vector<BYTE>>::Failure(
                CreateWindowsError("GetSystemFirmwareTable size query", error)
            );
        }

        std::vector<BYTE> buffer(bufferSize);

        const DWORD bytesWritten = ::GetSystemFirmwareTable(
            signature,
            0,
            buffer.data(),
            bufferSize
        );

        if (bytesWritten == 0 || bytesWritten != bufferSize) [[unlikely]] {
            const DWORD error = ::GetLastError();
            return domain::Expected<std::vector<BYTE>>::Failure(
                CreateWindowsError("GetSystemFirmwareTable data read", error)
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
