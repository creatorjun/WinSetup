#include "WindowsLogger.h"
#include <sstream>
#include <iomanip>
#include <Windows.h>

namespace winsetup::infrastructure {

    WindowsLogger::WindowsLogger() noexcept
        : minimumLevel_(domain::LogLevel::Info)
        , isInitialized_(false) {
    }

    WindowsLogger::~WindowsLogger() {
        Close();
    }

    bool WindowsLogger::Initialize(const std::wstring& logFilePath) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            if (isInitialized_) {
                Close();
            }

            logFilePath_ = logFilePath;

            fileStream_.open(logFilePath_, std::ios::out | std::ios::app);
            if (!fileStream_.is_open()) {
                return false;
            }

            fileStream_.imbue(std::locale(""));

            isInitialized_ = true;

            return true;
        }
        catch (...) {
            isInitialized_ = false;
            return false;
        }
    }

    void WindowsLogger::Log(
        domain::LogLevel level,
        std::wstring_view message
    ) noexcept {
        Log(level, message, L"");
    }

    void WindowsLogger::Log(
        domain::LogLevel level,
        std::wstring_view message,
        std::wstring_view category
    ) noexcept {
        if (!ShouldLog(level)) {
            return;
        }

        WriteLogEntry(level, message, category);
    }

    void WindowsLogger::Trace(std::wstring_view message) noexcept {
        Log(domain::LogLevel::Trace, message);
    }

    void WindowsLogger::Debug(std::wstring_view message) noexcept {
        Log(domain::LogLevel::Debug, message);
    }

    void WindowsLogger::Info(std::wstring_view message) noexcept {
        Log(domain::LogLevel::Info, message);
    }

    void WindowsLogger::Warning(std::wstring_view message) noexcept {
        Log(domain::LogLevel::Warning, message);
    }

    void WindowsLogger::Error(std::wstring_view message) noexcept {
        Log(domain::LogLevel::Error, message);
    }

    void WindowsLogger::Fatal(std::wstring_view message) noexcept {
        Log(domain::LogLevel::Fatal, message);
    }

    void WindowsLogger::SetMinimumLevel(domain::LogLevel level) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        minimumLevel_ = level;
    }

    domain::LogLevel WindowsLogger::GetMinimumLevel() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return minimumLevel_;
    }

    void WindowsLogger::Flush() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            if (isInitialized_ && fileStream_.is_open()) {
                fileStream_.flush();
            }
        }
        catch (...) {
        }
    }

    void WindowsLogger::Close() noexcept {
        try {
            if (fileStream_.is_open()) {
                fileStream_.flush();
                fileStream_.close();
            }
            isInitialized_ = false;
        }
        catch (...) {
        }
    }

    bool WindowsLogger::IsInitialized() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return isInitialized_;
    }

    std::wstring WindowsLogger::GetLogFilePath() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return logFilePath_;
    }

    void WindowsLogger::WriteLogEntry(
        domain::LogLevel level,
        std::wstring_view message,
        std::wstring_view category
    ) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            if (!isInitialized_ || !fileStream_.is_open()) {
                return;
            }

            const auto now = std::chrono::system_clock::now();
            const auto timestamp = FormatTimestamp(now);
            const auto levelStr = domain::LogLevelToWideString(level);

            fileStream_ << L"[" << timestamp << L"] ";
            fileStream_ << L"[" << levelStr << L"] ";

            if (!category.empty()) {
                fileStream_ << L"[" << category << L"] ";
            }

            fileStream_ << message << L"\n";

            if (level >= domain::LogLevel::Error) {
                fileStream_.flush();
            }
        }
        catch (...) {
        }
    }

    std::wstring WindowsLogger::FormatTimestamp(
        const std::chrono::system_clock::time_point& timePoint
    ) const noexcept {
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

    bool WindowsLogger::ShouldLog(domain::LogLevel level) const noexcept {
        return domain::ShouldLog(level, minimumLevel_);
    }

}
