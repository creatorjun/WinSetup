// src/application/events/LogEvent.h 
#pragma once

#include <string>
#include <chrono>

namespace winsetup::application {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    struct LogEvent {
        LogLevel level;
        std::string message;
        std::string category;
        std::chrono::system_clock::time_point timestamp;

        LogEvent()
            : level(LogLevel::Info)
            , timestamp(std::chrono::system_clock::now()) {
        }

        LogEvent(
            LogLevel lvl,
            std::string msg,
            std::string cat = ""
        ) : level(lvl)
            , message(std::move(msg))
            , category(std::move(cat))
            , timestamp(std::chrono::system_clock::now()) {
        }

        static LogEvent Trace(std::string message, std::string category = "") {
            return LogEvent(LogLevel::Trace, std::move(message), std::move(category));
        }

        static LogEvent Debug(std::string message, std::string category = "") {
            return LogEvent(LogLevel::Debug, std::move(message), std::move(category));
        }

        static LogEvent Info(std::string message, std::string category = "") {
            return LogEvent(LogLevel::Info, std::move(message), std::move(category));
        }

        static LogEvent Warning(std::string message, std::string category = "") {
            return LogEvent(LogLevel::Warning, std::move(message), std::move(category));
        }

        static LogEvent Error(std::string message, std::string category = "") {
            return LogEvent(LogLevel::Error, std::move(message), std::move(category));
        }

        static LogEvent Critical(std::string message, std::string category = "") {
            return LogEvent(LogLevel::Critical, std::move(message), std::move(category));
        }
    };

}
