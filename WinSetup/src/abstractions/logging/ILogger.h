#pragma once

#include <string>
#include <string_view>

namespace winsetup::abstractions {

    enum class LogLevel : uint8_t {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5
    };

    [[nodiscard]] constexpr const wchar_t* LogLevelToString(LogLevel level) noexcept {
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

    class ILogger {
    public:
        virtual ~ILogger() = default;

        virtual void Log(
            LogLevel level,
            std::wstring_view message
        ) noexcept = 0;

        virtual void Log(
            LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) noexcept = 0;

        virtual void Trace(std::wstring_view message) noexcept {
            Log(LogLevel::Trace, message);
        }

        virtual void Debug(std::wstring_view message) noexcept {
            Log(LogLevel::Debug, message);
        }

        virtual void Info(std::wstring_view message) noexcept {
            Log(LogLevel::Info, message);
        }

        virtual void Warning(std::wstring_view message) noexcept {
            Log(LogLevel::Warning, message);
        }

        virtual void Error(std::wstring_view message) noexcept {
            Log(LogLevel::Error, message);
        }

        virtual void Fatal(std::wstring_view message) noexcept {
            Log(LogLevel::Fatal, message);
        }

        virtual void SetMinimumLevel(LogLevel level) noexcept = 0;
        virtual LogLevel GetMinimumLevel() const noexcept = 0;

        virtual void Flush() noexcept = 0;
        virtual void Close() noexcept = 0;
    };

}
