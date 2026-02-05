#pragma once

#include <cstdint>

namespace winsetup::abstractions {

    enum class LogLevel : uint8_t {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5
    };

    constexpr const wchar_t* LogLevelToString(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Trace:   return L"TRACE";
        case LogLevel::Debug:   return L"DEBUG";
        case LogLevel::Info:    return L"INFO";
        case LogLevel::Warning: return L"WARN";
        case LogLevel::Error:   return L"ERROR";
        case LogLevel::Fatal:   return L"FATAL";
        default:                return L"UNKNOWN";
        }
    }

    constexpr bool operator<(LogLevel lhs, LogLevel rhs) noexcept {
        return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs);
    }

    constexpr bool operator<=(LogLevel lhs, LogLevel rhs) noexcept {
        return static_cast<uint8_t>(lhs) <= static_cast<uint8_t>(rhs);
    }

    constexpr bool operator>(LogLevel lhs, LogLevel rhs) noexcept {
        return static_cast<uint8_t>(lhs) > static_cast<uint8_t>(rhs);
    }

    constexpr bool operator>=(LogLevel lhs, LogLevel rhs) noexcept {
        return static_cast<uint8_t>(lhs) >= static_cast<uint8_t>(rhs);
    }

}
