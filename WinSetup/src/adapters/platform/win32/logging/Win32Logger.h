// src/adapters/platform/win32/logging/Win32Logger.h

#pragma once

#include <abstractions/infrastructure/logging/ILogger.h>
#include <windows.h>
#include <string>
#include <mutex>
#include <array>
#include <atomic>

namespace winsetup::adapters::platform {

    class Win32Logger : public abstractions::ILogger {
    public:
        explicit Win32Logger(const std::wstring& logFilePath = L"WinSetup.log");
        ~Win32Logger() override;

        Win32Logger(const Win32Logger&) = delete;
        Win32Logger& operator=(const Win32Logger&) = delete;

        void Log(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location
        ) override;

    private:
        static constexpr size_t BUFFER_SIZE = 8192;
        static constexpr size_t FLUSH_THRESHOLD = 6144;

        void FlushBuffer();
        void WriteToFile(const std::wstring& entry);
        const wchar_t* GetLevelString(abstractions::LogLevel level) const noexcept;
        void FormatTimestamp(wchar_t* buffer, size_t bufferSize) const noexcept;

        HANDLE m_hFile;
        std::mutex m_mutex;
        std::wstring m_buffer;
        std::atomic<bool> m_shouldFlush;
    };

}
