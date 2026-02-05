#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include "../../abstractions/logging/ILogger.h"
#include "../../domain/primitives/LogLevel.h"

namespace winsetup::infrastructure {

    class WindowsLogger : public abstractions::ILogger {
    public:
        WindowsLogger() noexcept;
        ~WindowsLogger() override;

        WindowsLogger(const WindowsLogger&) = delete;
        WindowsLogger& operator=(const WindowsLogger&) = delete;
        WindowsLogger(WindowsLogger&&) noexcept = default;
        WindowsLogger& operator=(WindowsLogger&&) noexcept = default;

        [[nodiscard]] bool Initialize(const std::wstring& logFilePath) noexcept;

        void Log(
            domain::LogLevel level,
            std::wstring_view message
        ) noexcept override;

        void Log(
            domain::LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) noexcept override;

        void Trace(std::wstring_view message) noexcept;
        void Debug(std::wstring_view message) noexcept;
        void Info(std::wstring_view message) noexcept;
        void Warning(std::wstring_view message) noexcept;
        void Error(std::wstring_view message) noexcept;
        void Fatal(std::wstring_view message) noexcept;

        void SetMinimumLevel(domain::LogLevel level) noexcept override;
        [[nodiscard]] domain::LogLevel GetMinimumLevel() const noexcept override;

        void Flush() noexcept override;
        void Close() noexcept override;

        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] std::wstring GetLogFilePath() const noexcept;

    private:
        void WriteLogEntry(
            domain::LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) noexcept;

        [[nodiscard]] std::wstring FormatTimestamp(
            const std::chrono::system_clock::time_point& timePoint
        ) const noexcept;

        [[nodiscard]] bool ShouldLog(domain::LogLevel level) const noexcept;

        mutable std::mutex mutex_;
        std::wofstream fileStream_;
        std::wstring logFilePath_;
        domain::LogLevel minimumLevel_;
        bool isInitialized_;
    };

}
