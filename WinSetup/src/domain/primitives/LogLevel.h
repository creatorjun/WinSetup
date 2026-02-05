#pragma once

#include <cstdint>

namespace winsetup::domain {

    enum class LogLevel : uint8_t {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5
    };

    [[nodiscard]] constexpr const wchar_t* LogLevelToWideString(LogLevel level) noexcept {
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

    [[nodiscard]] constexpr const char* LogLevelToString(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
        }
    }

    [[nodiscard]] constexpr bool IsValidLogLevel(LogLevel level) noexcept {
        return level >= LogLevel::Trace && level <= LogLevel::Fatal;
    }

    [[nodiscard]] constexpr bool ShouldLog(LogLevel messageLevel, LogLevel minimumLevel) noexcept {
        return static_cast<uint8_t>(messageLevel) >= static_cast<uint8_t>(minimumLevel);
    }

}
