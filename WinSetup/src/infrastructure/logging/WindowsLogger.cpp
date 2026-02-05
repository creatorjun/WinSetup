#include "WindowsLogger.h"
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace winsetup::infrastructure {

    struct WindowsLogger::Impl {
        std::wofstream fileStream;
        std::wstring logFilePath;
        abstractions::LogLevel minimumLevel{ abstractions::LogLevel::Info };
        mutable std::mutex mutex;
        bool isInitialized{ false };
        size_t logCount{ 0 };
        HANDLE consoleHandle{ INVALID_HANDLE_VALUE };
        WORD originalConsoleAttributes{ 0 };
    };

    WindowsLogger::WindowsLogger() noexcept
        : pImpl_(std::make_unique<Impl>()) {
        pImpl_->consoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (pImpl_->consoleHandle != INVALID_HANDLE_VALUE) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (::GetConsoleScreenBufferInfo(pImpl_->consoleHandle, &csbi)) {
                pImpl_->originalConsoleAttributes = csbi.wAttributes;
            }
        }
    }

    WindowsLogger::~WindowsLogger() {
        Close();
    }

    bool WindowsLogger::Initialize(std::wstring_view logFilePath) noexcept {
        std::lock_guard lock(pImpl_->mutex);

        if (pImpl_->isInitialized) {
            Close();
        }

        try {
            pImpl_->logFilePath = logFilePath;

            const std::filesystem::path logPath(logFilePath);
            if (const auto parentPath = logPath.parent_path(); !parentPath.empty()) {
                std::filesystem::create_directories(parentPath);
            }

            pImpl_->fileStream.open(
                pImpl_->logFilePath,
                std::ios::out | std::ios::app
            );

            if (!pImpl_->fileStream.is_open()) {
                return false;
            }

            pImpl_->fileStream.imbue(std::locale(""));

            pImpl_->isInitialized = true;
            pImpl_->logCount = 0;

            return true;
        }
        catch (...) {
            pImpl_->isInitialized = false;
            return false;
        }
    }

    void WindowsLogger::Log(
        abstractions::LogLevel level,
        std::wstring_view message
    ) noexcept {
        Log(level, message, L"");
    }

    void WindowsLogger::Log(
        abstractions::LogLevel level,
        std::wstring_view message,
        std::wstring_view category
    ) noexcept {
        if (!ShouldLog(level)) {
            return;
        }

        try {
            const std::wstring logLine = FormatLogMessage(level, message, category);

            {
                std::lock_guard lock(pImpl_->mutex);
                WriteToFile(logLine);
                WriteToDebugOutput(logLine);
                ++pImpl_->logCount;
            }
        }
        catch (...) {
        }
    }

    void WindowsLogger::SetMinimumLevel(abstractions::LogLevel level) noexcept {
        std::lock_guard lock(pImpl_->mutex);
        pImpl_->minimumLevel = level;
    }

    abstractions::LogLevel WindowsLogger::GetMinimumLevel() const noexcept {
        std::lock_guard lock(pImpl_->mutex);
        return pImpl_->minimumLevel;
    }

    void WindowsLogger::Flush() noexcept {
        std::lock_guard lock(pImpl_->mutex);
        if (pImpl_->isInitialized && pImpl_->fileStream.is_open()) {
            pImpl_->fileStream.flush();
        }
    }

    void WindowsLogger::Close() noexcept {
        std::lock_guard lock(pImpl_->mutex);
        if (pImpl_->isInitialized) {
            if (pImpl_->fileStream.is_open()) {
                pImpl_->fileStream.flush();
                pImpl_->fileStream.close();
            }
            pImpl_->isInitialized = false;
        }
    }

    bool WindowsLogger::IsInitialized() const noexcept {
        std::lock_guard lock(pImpl_->mutex);
        return pImpl_->isInitialized;
    }

    std::wstring WindowsLogger::GetLogFilePath() const noexcept {
        std::lock_guard lock(pImpl_->mutex);
        return pImpl_->logFilePath;
    }

    size_t WindowsLogger::GetLogCount() const noexcept {
        std::lock_guard lock(pImpl_->mutex);
        return pImpl_->logCount;
    }

    bool WindowsLogger::ShouldLog(abstractions::LogLevel level) const noexcept {
        std::lock_guard lock(pImpl_->mutex);
        return pImpl_->isInitialized && level >= pImpl_->minimumLevel;
    }

    void WindowsLogger::WriteToFile(std::wstring_view logLine) noexcept {
        if (pImpl_->fileStream.is_open()) {
            pImpl_->fileStream << logLine << L'\n';
        }
    }

    void WindowsLogger::WriteToDebugOutput(std::wstring_view logLine) noexcept {
        ::OutputDebugStringW((std::wstring(logLine) + L"\n").c_str());
    }

    std::wstring WindowsLogger::FormatLogMessage(
        abstractions::LogLevel level,
        std::wstring_view message,
        std::wstring_view category
    ) const noexcept {
        std::wostringstream oss;
        oss << L"[" << GetCurrentTimestamp() << L"] ";
        oss << L"[" << abstractions::LogLevelToString(level) << L"] ";

        if (!category.empty()) {
            oss << L"[" << category << L"] ";
        }

        oss << message;
        return oss.str();
    }

    std::wstring WindowsLogger::GetCurrentTimestamp() const noexcept {
        const auto now = std::chrono::system_clock::now();
        const auto nowTime = std::chrono::system_clock::to_time_t(now);
        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        std::tm timeInfo;
        localtime_s(&timeInfo, &nowTime);

        std::wostringstream oss;
        oss << std::put_time(&timeInfo, L"%Y-%m-%d %H:%M:%S");
        oss << L'.' << std::setfill(L'0') << std::setw(3) << nowMs.count();

        return oss.str();
    }

    WORD WindowsLogger::GetConsoleColor(abstractions::LogLevel level) noexcept {
        constexpr WORD DEFAULT_COLOR = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        constexpr WORD TRACE_COLOR = FOREGROUND_INTENSITY;
        constexpr WORD DEBUG_COLOR = FOREGROUND_GREEN | FOREGROUND_BLUE;
        constexpr WORD INFO_COLOR = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD WARNING_COLOR = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        constexpr WORD ERROR_COLOR = FOREGROUND_RED | FOREGROUND_INTENSITY;
        constexpr WORD FATAL_COLOR = FOREGROUND_RED | BACKGROUND_INTENSITY;

        switch (level) {
        case abstractions::LogLevel::Trace:   return TRACE_COLOR;
        case abstractions::LogLevel::Debug:   return DEBUG_COLOR;
        case abstractions::LogLevel::Info:    return INFO_COLOR;
        case abstractions::LogLevel::Warning: return WARNING_COLOR;
        case abstractions::LogLevel::Error:   return ERROR_COLOR;
        case abstractions::LogLevel::Fatal:   return FATAL_COLOR;
        default:                              return DEFAULT_COLOR;
        }
    }

}
