#include "LogFormatter.h"
#include <sstream>
#include <iomanip>
#include <thread>

namespace winsetup::infrastructure {

    std::wstring LogFormatter::Format(
        domain::LogLevel level,
        std::wstring_view message,
        std::wstring_view category
    ) noexcept {
        const auto now = std::chrono::system_clock::now();
        return Format(level, message, category, now);
    }

    std::wstring LogFormatter::Format(
        domain::LogLevel level,
        std::wstring_view message,
        std::wstring_view category,
        const std::chrono::system_clock::time_point& timestamp
    ) noexcept {
        try {
            std::wostringstream oss;

            oss << L"[" << FormatTimestamp(timestamp) << L"] ";
            oss << L"[" << domain::LogLevelToWideString(level) << L"] ";

            if (!category.empty()) {
                oss << L"[" << category << L"] ";
            }

            oss << message;

            return oss.str();
        }
        catch (...) {
            return L"[ERROR] Failed to format log message";
        }
    }

    std::wstring LogFormatter::FormatTimestamp(
        const std::chrono::system_clock::time_point& timePoint
    ) noexcept {
        try {
            const auto timeT = std::chrono::system_clock::to_time_t(timePoint);
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                timePoint.time_since_epoch()
            ) % 1000;

            tm localTime;
            if (localtime_s(&localTime, &timeT) != 0) {
                return L"INVALID_TIME";
            }

            std::wostringstream oss;
            oss << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S");
            oss << L"." << std::setfill(L'0') << std::setw(3) << ms.count();

            return oss.str();
        }
        catch (...) {
            return L"TIMESTAMP_ERROR";
        }
    }

    std::wstring LogFormatter::FormatWithThreadId(
        domain::LogLevel level,
        std::wstring_view message,
        std::wstring_view category
    ) noexcept {
        try {
            const auto threadId = std::this_thread::get_id();
            std::wostringstream oss;

            oss << L"[TID:" << threadId << L"] ";
            oss << Format(level, message, category);

            return oss.str();
        }
        catch (...) {
            return L"[ERROR] Failed to format log message with thread ID";
        }
    }

}
