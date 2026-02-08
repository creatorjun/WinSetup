#pragma once

#include <Windows.h>
#include <string>
#include <string_view>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <abstractions/logging/ILogger.h>
#include <abstractions/logging/LogLevel.h>

namespace winsetup::infrastructure {

    class WindowsLogger final : public abstractions::ILogger {
    public:
        WindowsLogger() noexcept;
        ~WindowsLogger() override;

        WindowsLogger(const WindowsLogger&) = delete;
        WindowsLogger& operator=(const WindowsLogger&) = delete;
        WindowsLogger(WindowsLogger&&) noexcept = delete;
        WindowsLogger& operator=(WindowsLogger&&) noexcept = delete;

        [[nodiscard]] bool Initialize(std::wstring_view logFilePath) noexcept;

        void Log(
            abstractions::LogLevel level,
            std::wstring_view message
        ) noexcept override;

        void Log(
            abstractions::LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) noexcept override;

        void SetMinimumLevel(abstractions::LogLevel level) noexcept override;
        [[nodiscard]] abstractions::LogLevel GetMinimumLevel() const noexcept override;

        void Flush() noexcept override;
        void Close() noexcept override;

        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] std::wstring GetLogFilePath() const noexcept;
        [[nodiscard]] size_t GetLogCount() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl_;

        [[nodiscard]] bool ShouldLog(abstractions::LogLevel level) const noexcept;
        void WriteToFile(std::wstring_view logLine) noexcept;
        void WriteToDebugOutput(std::wstring_view logLine) noexcept;
        [[nodiscard]] std::wstring FormatLogMessage(
            abstractions::LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) const noexcept;
        [[nodiscard]] std::wstring GetCurrentTimestamp() const noexcept;
        [[nodiscard]] static WORD GetConsoleColor(abstractions::LogLevel level) noexcept;
    };

}
