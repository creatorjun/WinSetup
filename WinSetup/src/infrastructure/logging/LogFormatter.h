#pragma once

#include <string>
#include "../../domain/entities/LogEntry.h"

namespace winsetup::infrastructure {

    class LogFormatter {
    public:
        LogFormatter() = default;
        ~LogFormatter() = default;

        LogFormatter(const LogFormatter&) = delete;
        LogFormatter& operator=(const LogFormatter&) = delete;
        LogFormatter(LogFormatter&&) noexcept = default;
        LogFormatter& operator=(LogFormatter&&) noexcept = default;

        [[nodiscard]] std::wstring Format(const domain::LogEntry& entry) const noexcept;

        [[nodiscard]] std::wstring FormatTimestamp(
            std::chrono::system_clock::time_point timestamp
        ) const noexcept;

    private:
        [[nodiscard]] std::wstring PadRight(
            const std::wstring& str,
            size_t width
        ) const noexcept;
    };

}
