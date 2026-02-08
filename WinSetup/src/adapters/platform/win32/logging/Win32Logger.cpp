// src/adapters/platform/win32/logging/Win32Logger.cpp

#include "Win32Logger.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace winsetup::adapters::platform {

    Win32Logger::Win32Logger(const std::wstring& logFilePath)
        : m_logFilePath(logFilePath)
    {
        m_logFile.open(logFilePath, std::ios::out | std::ios::app);
    }

    Win32Logger::~Win32Logger() {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }

    void Win32Logger::Log(
        abstractions::LogLevel level,
        const std::wstring& message,
        const std::source_location& location)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::wstring logEntry = L"[" + GetTimestamp() + L"] " +
            L"[" + GetLevelString(level) + L"] " +
            message + L"\n";

        if (m_logFile.is_open()) {
            m_logFile << logEntry;
            m_logFile.flush();
        }

        OutputDebugStringW(logEntry.c_str());
    }

    std::wstring Win32Logger::GetLevelString(abstractions::LogLevel level) const {
        switch (level) {
        case abstractions::LogLevel::Trace:   return L"TRACE";
        case abstractions::LogLevel::Debug:   return L"DEBUG";
        case abstractions::LogLevel::Info:    return L"INFO";
        case abstractions::LogLevel::Warning: return L"WARN";
        case abstractions::LogLevel::Error:   return L"ERROR";
        case abstractions::LogLevel::Fatal:   return L"FATAL";
        default:                              return L"UNKNOWN";
        }
    }

    std::wstring Win32Logger::GetTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm;
        localtime_s(&tm, &time);

        std::wostringstream oss;
        oss << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

}
