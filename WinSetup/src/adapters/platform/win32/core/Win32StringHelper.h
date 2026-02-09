// src/adapters/platform/win32/core/Win32StringHelper.h
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
            wchar_t buffer[16];
            swprintf_s(buffer, 16, L"%u", value);
            return std::wstring(buffer);
        }

        static std::wstring UInt64ToString(uint64_t value) {
            wchar_t buffer[32];
            swprintf_s(buffer, 32, L"%llu", value);
            return std::wstring(buffer);
        }
    };

}
