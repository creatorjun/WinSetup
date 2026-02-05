#pragma once

#include <string>
#include <string_view>
#include "LogLevel.h"

namespace winsetup::abstractions {

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

        virtual void SetMinimumLevel(LogLevel level) noexcept = 0;
        [[nodiscard]] virtual LogLevel GetMinimumLevel() const noexcept = 0;

        virtual void Flush() noexcept = 0;
        virtual void Close() noexcept = 0;

        void Trace(std::wstring_view message) noexcept {
            Log(LogLevel::Trace, message);
        }

        void Debug(std::wstring_view message) noexcept {
            Log(LogLevel::Debug, message);
        }

        void Info(std::wstring_view message) noexcept {
            Log(LogLevel::Info, message);
        }

        void Warning(std::wstring_view message) noexcept {
            Log(LogLevel::Warning, message);
        }

        void Error(std::wstring_view message) noexcept {
            Log(LogLevel::Error, message);
        }

        void Fatal(std::wstring_view message) noexcept {
            Log(LogLevel::Fatal, message);
        }

        void Trace(std::wstring_view message, std::wstring_view category) noexcept {
            Log(LogLevel::Trace, message, category);
        }

        void Debug(std::wstring_view message, std::wstring_view category) noexcept {
            Log(LogLevel::Debug, message, category);
        }

        void Info(std::wstring_view message, std::wstring_view category) noexcept {
            Log(LogLevel::Info, message, category);
        }

        void Warning(std::wstring_view message, std::wstring_view category) noexcept {
            Log(LogLevel::Warning, message, category);
        }

        void Error(std::wstring_view message, std::wstring_view category) noexcept {
            Log(LogLevel::Error, message, category);
        }

        void Fatal(std::wstring_view message, std::wstring_view category) noexcept {
            Log(LogLevel::Fatal, message, category);
        }
    };

}
