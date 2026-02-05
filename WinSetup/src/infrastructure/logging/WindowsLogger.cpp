// src/infrastructure/logging/WindowsLogger.cpp 
#include "WindowsLogger.h"
#include <Windows.h>
#include <filesystem>
#include <sstream>

namespace winsetup::infrastructure {

    WindowsLogger::WindowsLogger(
        const std::wstring& logFilePath,
        LogTarget targets,
        abstractions::LogLevel minLevel
    )
        : logFilePath_(logFilePath)
        , targets_(targets)
        , minLevel_(minLevel)
        , formatter_()
        , maxFileSize_(DEFAULT_MAX_FILE_SIZE)
        , rotationEnabled_(true)
        , rotationCount_(0) {

        if (targets_ & LogTarget::File) {
            logFile_.open(logFilePath_, std::ios::out | std::ios::app);
            logFile_.imbue(std::locale(""));
        }
    }

    WindowsLogger::~WindowsLogger() {
        Close();
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
        if (level < minLevel_) {
            return;
        }

        try {
            domain::LogEntry entry(
                level,
                std::wstring(message),
                std::wstring(category)
            );

            std::wstring formatted = formatter_.Format(entry);

            std::lock_guard<std::mutex> lock(mutex_);

            if (targets_ & LogTarget::File) {
                WriteToFile(formatted);
            }

            if (targets_ & LogTarget::DebugOutput) {
                WriteToDebugOutput(formatted);
            }

            if (targets_ & LogTarget::Console) {
                WriteToConsole(formatted);
            }
        }
        catch (...) {
        }
    }

    void WindowsLogger::SetMinimumLevel(abstractions::LogLevel level) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        minLevel_ = level;
    }

    abstractions::LogLevel WindowsLogger::GetMinimumLevel() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return minLevel_;
    }

    void WindowsLogger::Flush() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.flush();
        }
    }

    void WindowsLogger::Close() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.flush();
            logFile_.close();
        }
    }

    void WindowsLogger::SetMaxFileSize(uint64_t bytes) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        maxFileSize_ = bytes;
    }

    void WindowsLogger::EnableRotation(bool enable) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        rotationEnabled_ = enable;
    }

    void WindowsLogger::WriteToFile(const std::wstring& formatted) noexcept {
        if (!logFile_.is_open()) {
            return;
        }

        if (rotationEnabled_ && ShouldRotate()) {
            RotateLogFile();
        }

        logFile_ << formatted << L"\n";
        logFile_.flush();
    }

    void WindowsLogger::WriteToDebugOutput(const std::wstring& formatted) noexcept {
        OutputDebugStringW((formatted + L"\n").c_str());
    }

    void WindowsLogger::WriteToConsole(const std::wstring& formatted) noexcept {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE) {
            return;
        }

        std::wstring output = formatted + L"\n";
        DWORD written = 0;
        WriteConsoleW(hConsole, output.c_str(), static_cast<DWORD>(output.length()), &written, nullptr);
    }

    void WindowsLogger::RotateLogFile() noexcept {
        if (!logFile_.is_open()) {
            return;
        }

        logFile_.close();

        try {
            for (uint32_t i = MAX_ROTATION_COUNT; i > 0; --i) {
                std::wstring oldPath = logFilePath_ + L"." + std::to_wstring(i);
                std::wstring newPath = logFilePath_ + L"." + std::to_wstring(i + 1);

                if (std::filesystem::exists(oldPath)) {
                    if (i == MAX_ROTATION_COUNT) {
                        std::filesystem::remove(oldPath);
                    }
                    else {
                        std::filesystem::rename(oldPath, newPath);
                    }
                }
            }

            std::wstring firstRotation = logFilePath_ + L".1";
            if (std::filesystem::exists(logFilePath_)) {
                std::filesystem::rename(logFilePath_, firstRotation);
            }
        }
        catch (...) {
        }

        logFile_.open(logFilePath_, std::ios::out | std::ios::app);
        logFile_.imbue(std::locale(""));
        rotationCount_ = 0;
    }

    bool WindowsLogger::ShouldRotate() const noexcept {
        return GetCurrentFileSize() >= maxFileSize_;
    }

    uint64_t WindowsLogger::GetCurrentFileSize() const noexcept {
        try {
            if (std::filesystem::exists(logFilePath_)) {
                return std::filesystem::file_size(logFilePath_);
            }
        }
        catch (...) {
        }
        return 0;
    }

    std::unique_ptr<abstractions::ILogger> CreateLogger(
        const std::wstring& logFilePath,
        LogTarget targets,
        abstractions::LogLevel minLevel
    ) {
        return std::make_unique<WindowsLogger>(logFilePath, targets, minLevel);
    }

}
