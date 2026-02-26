#pragma once
#include "abstractions/infrastructure/logging/ILogger.h"
#include "adapters/platform/win32/memory/UniqueHandle.h"
#include <string>
#include <vector>
#include <Windows.h>

namespace winsetup::adapters::platform {

    class Win32Logger final : public abstractions::ILogger {
    public:
        explicit Win32Logger(const std::wstring& logFilePath = L"log/log.txt");
        ~Win32Logger() override;

        Win32Logger(const Win32Logger&) = delete;
        Win32Logger& operator=(const Win32Logger&) = delete;

        void Log(
            abstractions::LogLevel      level,
            const std::wstring& message,
            const std::source_location& location) override;

        void Flush();

    private:
        struct LogEntry {
            abstractions::LogLevel level;
            std::wstring           message;
            wchar_t                timestamp[32];
        };

        void WriterLoop();
        void FlushBatch(std::vector<LogEntry>& batch);
        void FormatAndWrite(const LogEntry& entry);

        [[nodiscard]] const wchar_t* GetLevelString(abstractions::LogLevel level) const noexcept;
        void FormatTimestamp(wchar_t* buffer, size_t bufferSize) const noexcept;
        [[nodiscard]] bool EnsureFileOpen();
        [[nodiscard]] bool EnsureDirectoryExists();

        UniqueHandle    mHFile;
        std::wstring    mLogFilePath;
        std::wstring    mWriteBuffer;

        std::vector<LogEntry>   mQueue;
        std::vector<LogEntry>   mFlushQueue;
        CRITICAL_SECTION        mQueueLock{};
        CONDITION_VARIABLE      mQueueCV{};

        UniqueHandle            mWriterThread;
        volatile bool           mShutdown = false;
        volatile bool           mFlushRequested = false;

        static DWORD WINAPI WriterThreadProc(LPVOID lpParam);

        static constexpr size_t kWriteBufferReserve = 65536;
        static constexpr size_t kQueueReserve = 256;
    };

} // namespace winsetup::adapters::platform
