#include "WindowsLogger.h"
#include <Windows.h>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace winsetup::infrastructure {

    WindowsLogger::WindowsLogger() = default;

    WindowsLogger::~WindowsLogger() {
        Close();
    }

    bool WindowsLogger::Initialize(const std::wstring& logFilePath) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (isInitialized_) {
            Close();
        }

        wchar_t exePath[MAX_PATH];
        ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring fullExePath(exePath);
        size_t pos = fullExePath.find_last_of(L"\\/");
        std::wstring exeDir = (pos != std::wstring::npos) ? fullExePath.substr(0, pos) : L".";

        std::wstring logDir = exeDir + L"\\log";

        if (!::CreateDirectoryW(logDir.c_str(), nullptr)) {
            DWORD error = ::GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }

        logFilePath_ = logDir + L"\\" + logFilePath;

        fileStream_.open(logFilePath_, std::ios::out | std::ios::trunc);
        if (!fileStream_.is_open()) {
            return false;
        }

        fileStream_.imbue(std::locale(""));

        isInitialized_ = true;

        return true;
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
        if (level < minimumLevel_) {
            return;
        }

        const auto formattedMessage = FormatLogMessage(level, message, category);

        WriteToFile(formattedMessage);
        WriteToDebugOutput(formattedMessage);
    }

    void WindowsLogger::SetMinimumLevel(abstractions::LogLevel level) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        minimumLevel_ = level;
    }

    abstractions::LogLevel WindowsLogger::GetMinimumLevel() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return minimumLevel_;
    }

    void WindowsLogger::Flush() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fileStream_.is_open()) {
            fileStream_.flush();
        }
    }

    void WindowsLogger::Close() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fileStream_.is_open()) {
            fileStream_.flush();
            fileStream_.close();
        }
        isInitialized_ = false;
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
        SYSTEMTIME st;
        ::GetLocalTime(&st);

        std::wostringstream oss;
        oss << std::setfill(L'0')
            << std::setw(4) << st.wYear << L"-"
            << std::setw(2) << st.wMonth << L"-"
            << std::setw(2) << st.wDay << L" "
            << std::setw(2) << st.wHour << L":"
            << std::setw(2) << st.wMinute << L":"
            << std::setw(2) << st.wSecond << L"."
            << std::setw(3) << st.wMilliseconds;

        return oss.str();
    }

    void WindowsLogger::WriteToFile(const std::wstring& message) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fileStream_.is_open()) {
            fileStream_ << message << std::endl;
        }
    }

    void WindowsLogger::WriteToDebugOutput(const std::wstring& message) noexcept {
        ::OutputDebugStringW((message + L"\n").c_str());
    }

}
