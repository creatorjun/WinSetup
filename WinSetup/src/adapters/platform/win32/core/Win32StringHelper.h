#pragma once

#include <string>
#include <cstdint>

namespace winsetup::adapters::platform {

    class Win32StringHelper {
    public:
        static std::wstring FormatDiskPath(uint32_t diskIndex) {
            wchar_t buffer[64];
            swprintf_s(buffer, 64, L"\\\\.\\PhysicalDrive%u", diskIndex);
            return std::wstring(buffer);
        }

        static std::wstring FormatMessage(const wchar_t* format, uint32_t value) {
            wchar_t buffer[256];
            swprintf_s(buffer, 256, format, value);
            return std::wstring(buffer);
        }

        static std::wstring FormatMessage(const wchar_t* format, uint32_t value1, uint32_t value2) {
            wchar_t buffer[256];
            swprintf_s(buffer, 256, format, value1, value2);
            return std::wstring(buffer);
        }

        static std::wstring ConcatWithCount(const wchar_t* prefix, size_t count, const wchar_t* suffix) {
            wchar_t buffer[256];
            swprintf_s(buffer, 256, L"%s%zu%s", prefix, count, suffix);
            return std::wstring(buffer);
        }

        static std::wstring UInt32ToString(uint32_t value) {
            return std::to_wstring(value);
        }

        static std::wstring UInt64ToString(uint64_t value) {
            return std::to_wstring(value);
        }
    };

}
