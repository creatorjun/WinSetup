#pragma once

#include <string>
#include <chrono>
#include <abstractions/logging/LogLevel.h>

namespace winsetup::application {

    struct LogEvent {
        abstractions::LogLevel level;
        std::string message;
        std::string category;
        std::chrono::system_clock::time_point timestamp;

        LogEvent()
            : level(abstractions::LogLevel::Info)
            , timestamp(std::chrono::system_clock::now()) {
        }

        LogEvent(
            abstractions::LogLevel lvl,
            std::string msg,
            std::string cat = ""
        )
            : level(lvl)
            , message(std::move(msg))
            , category(std::move(cat))
            , timestamp(std::chrono::system_clock::now()) {
        }

        static LogEvent Trace(std::string message, std::string category = "") {
            return LogEvent(abstractions::LogLevel::Trace, std::move(message), std::move(category));
        }

        static LogEvent Debug(std::string message, std::string category = "") {
            return LogEvent(abstractions::LogLevel::Debug, std::move(message), std::move(category));
        }

        static LogEvent Info(std::string message, std::string category = "") {
            return LogEvent(abstractions::LogLevel::Info, std::move(message), std::move(category));
        }

        static LogEvent Warning(std::string message, std::string category = "") {
            return LogEvent(abstractions::LogLevel::Warning, std::move(message), std::move(category));
        }

        static LogEvent Error(std::string message, std::string category = "") {
            return LogEvent(abstractions::LogLevel::Error, std::move(message), std::move(category));
        }

        static LogEvent Fatal(std::string message, std::string category = "") {
            return LogEvent(abstractions::LogLevel::Fatal, std::move(message), std::move(category));
        }
    };

}
