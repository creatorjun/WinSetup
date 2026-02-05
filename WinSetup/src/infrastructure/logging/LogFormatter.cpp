#include "LogFormatter.h"
#include <sstream>
#include <iomanip>
#include <Windows.h>

namespace winsetup::infrastructure {

    std::wstring LogFormatter::Format(const domain::LogEntry& entry) const noexcept {
        std::wostringstream oss;

        oss << L"[" << FormatTimestamp(entry.GetTimestamp()) << L"] ";

        oss << L"[" << PadRight(abstractions::LogLevelToString(entry.GetLevel()), 5) << L"] ";

        oss << L"[" << std::setw(5) << std::setfill(L'0') << entry.GetThreadId() << L"] ";

        if (entry.HasCategory()) {
            oss << L"[" << entry.GetCategory() << L"] ";
        }

        oss << entry.GetLogMessage();

        return oss.str();
    }

    std::wstring LogFormatter::FormatTimestamp(
        std::chrono::system_clock::time_point timestamp
    ) const noexcept {
        auto timeT = std::chrono::system_clock::to_time_t(timestamp);

        SYSTEMTIME st;
        FILETIME ft;

        auto duration = timestamp.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration - seconds);

        tm localTime;
        if (localtime_s(&localTime, &timeT) != 0) {
            return L"0000-00-00 00:00:00.000";
        }

        std::wostringstream oss;
        oss << std::setfill(L'0')
            << std::setw(4) << (localTime.tm_year + 1900) << L"-"
            << std::setw(2) << (localTime.tm_mon + 1) << L"-"
            << std::setw(2) << localTime.tm_mday << L" "
            << std::setw(2) << localTime.tm_hour << L":"
            << std::setw(2) << localTime.tm_min << L":"
            << std::setw(2) << localTime.tm_sec << L"."
            << std::setw(3) << millis.count();

        return oss.str();
    }

    std::wstring LogFormatter::PadRight(const std::wstring& str, size_t width) const noexcept {
        if (str.length() >= width) {
            return str;
        }
        return str + std::wstring(width - str.length(), L' ');
    }

}
