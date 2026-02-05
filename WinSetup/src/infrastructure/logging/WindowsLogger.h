// src/infrastructure/logging/WindowsLogger.h 
#pragma once

#include <memory>
#include <mutex>
#include <fstream>
#include "../../abstractions/logging/ILogger.h"
#include "../../domain/entities/LogEntry.h"
#include "LogFormatter.h"

namespace winsetup::infrastructure {

    enum class LogTarget : uint8_t {
        None = 0,
        File = 1 << 0,
        DebugOutput = 1 << 1,
        Console = 1 << 2,
        All = File | DebugOutput | Console
    };

    inline LogTarget operator|(LogTarget a, LogTarget b) {
        return static_cast<LogTarget>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
            );
    }

    inline bool operator&(LogTarget a, LogTarget b) {
        return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
    }

    class WindowsLogger : public abstractions::ILogger {
    public:
        explicit WindowsLogger(
            const std::wstring& logFilePath,
            LogTarget targets = LogTarget::All,
            abstractions::LogLevel minLevel = abstractions::LogLevel::Info
        );

        ~WindowsLogger() override;

        WindowsLogger(const WindowsLogger&) = delete;
        WindowsLogger& operator=(const WindowsLogger&) = delete;
        WindowsLogger(WindowsLogger&&) noexcept = delete;
        WindowsLogger& operator=(WindowsLogger&&) noexcept = delete;

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
        abstractions::LogLevel GetMinimumLevel() const noexcept override;

        void Flush() noexcept override;
        void Close() noexcept override;

        void SetMaxFileSize(uint64_t bytes) noexcept;
        void EnableRotation(bool enable) noexcept;

    private:
        void WriteToFile(const std::wstring& formatted) noexcept;
        void WriteToDebugOutput(const std::wstring& formatted) noexcept;
        void WriteToConsole(const std::wstring& formatted) noexcept;

        void RotateLogFile() noexcept;
        bool ShouldRotate() const noexcept;

        [[nodiscard]] uint64_t GetCurrentFileSize() const noexcept;

        std::wstring logFilePath_;
        std::wofstream logFile_;
        LogTarget targets_;
        abstractions::LogLevel minLevel_;
        LogFormatter formatter_;

        mutable std::mutex mutex_;

        uint64_t maxFileSize_;
        bool rotationEnabled_;
        uint32_t rotationCount_;

        static constexpr uint64_t DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024;
        static constexpr uint32_t MAX_ROTATION_COUNT = 5;
    };

    std::unique_ptr<abstractions::ILogger> CreateLogger(
        const std::wstring& logFilePath,
        LogTarget targets = LogTarget::All,
        abstractions::LogLevel minLevel = abstractions::LogLevel::Info
    );

}
