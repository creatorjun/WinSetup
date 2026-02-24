#include "adapters/platform/win32/logging/Win32Logger.h"
#include "adapters/platform/win32/core/Win32HandleFactory.h"
#include <Windows.h>
#include <chrono>

namespace winsetup::adapters::platform {

    Win32Logger::Win32Logger(const std::wstring& logFilePath)
        : mLogFilePath(logFilePath)
    {
        mBuffer.reserve(kBufferSize);

        if (EnsureFileOpen() && mHFile) {
            std::wstring initMsg = L"[INIT] Win32Logger initialized\r\n";
            DWORD bytesWritten = 0;
            WriteFile(
                Win32HandleFactory::ToWin32Handle(mHFile),
                initMsg.data(),
                static_cast<DWORD>(initMsg.size() * sizeof(wchar_t)),
                &bytesWritten,
                nullptr);
            FlushFileBuffers(Win32HandleFactory::ToWin32Handle(mHFile));
        }
    }

    Win32Logger::~Win32Logger()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        FlushBufferUnsafe();
    }

    bool Win32Logger::EnsureDirectoryExists()
    {
        const size_t lastSlash = mLogFilePath.find_last_of(L"\\/");
        if (lastSlash == std::wstring::npos)
            return true;

        const std::wstring dirPath = mLogFilePath.substr(0, lastSlash);

        const DWORD attribs = GetFileAttributesW(dirPath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
            return (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (!CreateDirectoryW(dirPath.c_str(), nullptr)) {
            const DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS)
                return false;
        }
        return true;
    }

    bool Win32Logger::EnsureFileOpen()
    {
        if (mHFile)
            return true;

        if (!EnsureDirectoryExists())
            return false;

        HANDLE hFile = CreateFileW(
            mLogFilePath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        SetFilePointer(hFile, 0, nullptr, FILE_END);
        mHFile = Win32HandleFactory::MakeHandle(hFile);
        return true;
    }

    bool Win32Logger::ShouldFlushImmediately(abstractions::LogLevel level) const noexcept
    {
        return level == abstractions::LogLevel::Error
            || level == abstractions::LogLevel::Fatal;
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

        const bool forceFlush = ShouldFlushImmediately(level);

        std::lock_guard<std::mutex> lock(mMutex);
        if (!EnsureFileOpen())
            return;
        WriteBufferedEntry(entry, forceFlush);
    }

    void Win32Logger::WriteBufferedEntry(const std::wstring& entry, bool forceFlush)
    {
        mBuffer += entry;

        if (forceFlush || mBuffer.size() >= kFlushThreshold)
            FlushBufferUnsafe();
    }

    void Win32Logger::Flush()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        FlushBufferUnsafe();
    }

    void Win32Logger::FlushBufferUnsafe()
    {
        if (!mHFile || mBuffer.empty())
            return;

        DWORD bytesWritten = 0;
        WriteFile(
            Win32HandleFactory::ToWin32Handle(mHFile),
            mBuffer.data(),
            static_cast<DWORD>(mBuffer.size() * sizeof(wchar_t)),
            &bytesWritten,
            nullptr);

        FlushFileBuffers(Win32HandleFactory::ToWin32Handle(mHFile));
        mBuffer.clear();
    }

    const wchar_t* Win32Logger::GetLevelString(abstractions::LogLevel level) const noexcept
    {
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

    void Win32Logger::FormatTimestamp(wchar_t* buffer, size_t bufferSize) const noexcept
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        swprintf_s(
            buffer,
            bufferSize,
            L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }

} // namespace winsetup::adapters::platform
