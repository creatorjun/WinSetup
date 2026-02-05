#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include "../../abstractions/logging/LogLevel.h"
#include "../../domain/entities/LogEntry.h"

namespace winsetup::infrastructure {

    class LogFormatter {
    public:
        LogFormatter() = delete;

        [[nodiscard]] static std::wstring Format(
            abstractions::LogLevel level,
            std::wstring_view message,
            std::wstring_view category = L""
        );

        [[nodiscard]] static std::wstring Format(
            const domain::LogEntry& entry
        );

        [[nodiscard]] static std::wstring Format(
            abstractions::LogLevel level,
            std::wstring_view message,
            std::wstring_view category,
            const std::chrono::system_clock::time_point& timestamp
        );

        [[nodiscard]] static std::wstring FormatTimestamp(
            const std::chrono::system_clock::time_point& timePoint
        );

        [[nodiscard]] static std::wstring FormatWithThreadId(
            abstractions::LogLevel level,
            std::wstring_view message,
            std::wstring_view category = L""
        );
    };

}
