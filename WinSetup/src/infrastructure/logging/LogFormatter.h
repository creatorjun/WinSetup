#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include "../../domain/primitives/LogLevel.h"

namespace winsetup::infrastructure {

    class LogFormatter {
    public:
        LogFormatter() = delete;

        [[nodiscard]] static std::wstring Format(
            domain::LogLevel level,
            std::wstring_view message,
            std::wstring_view category = L""
        ) noexcept;

        [[nodiscard]] static std::wstring Format(
            domain::LogLevel level,
            std::wstring_view message,
            std::wstring_view category,
            const std::chrono::system_clock::time_point& timestamp
        ) noexcept;

        [[nodiscard]] static std::wstring FormatTimestamp(
            const std::chrono::system_clock::time_point& timePoint
        ) noexcept;

        [[nodiscard]] static std::wstring FormatWithThreadId(
            domain::LogLevel level,
            std::wstring_view message,
            std::wstring_view category = L""
        ) noexcept;
    };

}
