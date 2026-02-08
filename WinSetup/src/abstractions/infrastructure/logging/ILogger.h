// src/abstractions/infrastructure/logging/ILogger.h

#pragma once

#include <string>
#include <source_location>
#include "LogLevel.h"

namespace winsetup::abstractions {

    class ILogger {
    public:
        virtual ~ILogger() = default;

        virtual void Log(
            LogLevel level,
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) = 0;

        void Trace(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Trace, message, location);
        }

        void Debug(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Debug, message, location);
        }

        void Info(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Info, message, location);
        }

        void Warning(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Warning, message, location);
        }

        void Error(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Error, message, location);
        }

        void Fatal(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Fatal, message, location);
        }
    };

}
