#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include "../../abstractions/logging/ILogger.h"

namespace winsetup::infrastructure {

    class WindowsLogger : public abstractions::ILogger {
    public:
        WindowsLogger();
        ~WindowsLogger() override;

        [[nodiscard]] bool Initialize(const std::wstring& logFilePath);

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

    private:
        [[nodiscard]] std::wstring FormatLogMessage(
            abstractions::LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) const noexcept;

        [[nodiscard]] std::wstring GetCurrentTimestamp() const noexcept;

        void WriteToFile(const std::wstring& message) noexcept;
        void WriteToDebugOutput(const std::wstring& message) noexcept;

        mutable std::mutex mutex_;
        std::wofstream fileStream_;
        std::wstring logFilePath_;
        abstractions::LogLevel minimumLevel_{ abstractions::LogLevel::Info };
        bool isInitialized_{ false };
    };

}
