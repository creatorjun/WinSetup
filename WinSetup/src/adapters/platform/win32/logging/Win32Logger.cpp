// src/adapters/platform/win32/logging/Win32Logger.cpp
#include "Win32Logger.h"
#include "../core/Win32HandleFactory.h"
#include <Windows.h>
#include <chrono>

namespace winsetup::adapters::platform {

    Win32Logger::Win32Logger(const std::wstring& logFilePath)
        : m_logFilePath(logFilePath)
    {
        m_buffer.reserve(BUFFER_SIZE);
        EnsureFileOpen();

        if (m_hFile) {
            std::wstring initMsg = L"[INIT] Win32Logger initialized\r\n";
            DWORD bytesWritten = 0;
            WriteFile(
                Win32HandleFactory::ToWin32Handle(m_hFile),
                initMsg.data(),
                static_cast<DWORD>(initMsg.size() * sizeof(wchar_t)),
                &bytesWritten,
                nullptr
            );
            FlushFileBuffers(Win32HandleFactory::ToWin32Handle(m_hFile));
        }
    }

    Win32Logger::~Win32Logger() {
        std::lock_guard<std::mutex> lock(m_mutex);
        FlushBufferUnsafe();
    }

    bool Win32Logger::EnsureFileOpen() {
        if (m_hFile) {
            return true;
        }

        HANDLE hFile = CreateFileW(
            m_logFilePath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            return false;
        }

        SetFilePointer(hFile, 0, nullptr, FILE_END);
        m_hFile = Win32HandleFactory::MakeHandle(hFile);
        return true;
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
        entry += timestamp;
        entry += L" [";
        entry += GetLevelString(level);
        entry += L"] ";
        entry += message;
        entry += L"\r\n";

        OutputDebugStringW(entry.c_str());

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (!EnsureFileOpen()) {
                return;
            }

            DWORD bytesWritten = 0;
            WriteFile(
                Win32HandleFactory::ToWin32Handle(m_hFile),
                entry.data(),
                static_cast<DWORD>(entry.size() * sizeof(wchar_t)),
                &bytesWritten,
                nullptr
            );

            FlushFileBuffers(Win32HandleFactory::ToWin32Handle(m_hFile));
        }
    }

    void Win32Logger::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        FlushBufferUnsafe();
    }

    void Win32Logger::FlushBufferUnsafe() {
        if (!m_hFile) {
            return;
        }
        FlushFileBuffers(Win32HandleFactory::ToWin32Handle(m_hFile));
    }

    const wchar_t* Win32Logger::GetLevelString(abstractions::LogLevel level) const noexcept {
        switch (level) {
        case abstractions::LogLevel::Trace:   return L"TRACE";
        case abstractions::LogLevel::Debug:   return L"DEBUG";
        case abstractions::LogLevel::Info:    return L"INFO ";
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
            L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
        );
    }

}
