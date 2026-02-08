// src/adapters/platform/win32/logging/Win32Logger.cpp
#include "Win32Logger.h"
#include "../core/Win32HandleFactory.h"
#include <Windows.h>
#include <chrono>

namespace winsetup::adapters::platform {

    Win32Logger::Win32Logger(const std::wstring& logFilePath)
        : m_shouldFlush(false)
    {
        m_buffer.reserve(BUFFER_SIZE);

        HANDLE hFile = CreateFileW(
            logFilePath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(hFile, 0, nullptr, FILE_END);
            m_hFile = Win32HandleFactory::MakeHandle(hFile);
        }
    }

    Win32Logger::~Win32Logger() {
        if (m_hFile) {
            FlushBuffer();
        }
    }

    void Win32Logger::Log(
        abstractions::LogLevel level,
        const std::wstring& message,
        const std::source_location& location)
    {
        wchar_t timestamp[32];
        FormatTimestamp(timestamp, 32);

        std::wstring entry;
        entry.reserve(256);
        entry += L"[";
        entry += timestamp;
        entry += L"] [";
        entry += GetLevelString(level);
        entry += L"] ";
        entry += message;
        entry += L"\r\n";

        OutputDebugStringW(entry.c_str());

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_buffer += entry;

            if (m_buffer.size() >= FLUSH_THRESHOLD) {
                FlushBuffer();
            }
        }
    }

    void Win32Logger::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        FlushBuffer();
    }

    void Win32Logger::FlushBuffer() {
        if (m_buffer.empty() || !m_hFile) {
            return;
        }

        const size_t byteCount = m_buffer.size() * sizeof(wchar_t);
        DWORD bytesWritten = 0;

        WriteFile(
            Win32HandleFactory::ToWin32Handle(m_hFile),
            m_buffer.data(),
            static_cast<DWORD>(byteCount),
            &bytesWritten,
            nullptr
        );

        m_buffer.clear();
    }

    const wchar_t* Win32Logger::GetLevelString(abstractions::LogLevel level) const noexcept {
        switch (level) {
        case abstractions::LogLevel::Trace:   return L"TRACE";
        case abstractions::LogLevel::Debug:   return L"DEBUG";
        case abstractions::LogLevel::Info:    return L"INFO ";
        case abstractions::LogLevel::Warning: return L"WARN ";
        case abstractions::LogLevel::Error:   return L"ERROR";
        case abstractions::LogLevel::Fatal:   return L"FATAL";
        default:                              return L"UNKNW";
        }
    }

    void Win32Logger::FormatTimestamp(wchar_t* buffer, size_t bufferSize) const noexcept {
        SYSTEMTIME st;
        GetLocalTime(&st);

        swprintf_s(
            buffer,
            bufferSize,
            L"%04d-%02d-%02d %02d:%02d:%02d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond
        );
    }

}
