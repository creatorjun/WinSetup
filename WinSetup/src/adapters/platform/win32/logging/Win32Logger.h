#pragma once
#include "abstractions/infrastructure/logging/ILogger.h"
#include "adapters/platform/win32/memory/UniqueHandle.h"
#include <string>
#include <mutex>

namespace winsetup::adapters::platform {

    class Win32Logger final : public abstractions::ILogger {
    public:
        explicit Win32Logger(const std::wstring& logFilePath = L"log/log.txt");
        ~Win32Logger() override;

        Win32Logger(const Win32Logger&) = delete;
        Win32Logger& operator=(const Win32Logger&) = delete;

        void Log(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location) override;

        void Flush();

    private:
        void FlushBufferUnsafe();
        void WriteBufferedEntry(const std::wstring& entry, bool forceFlush);

        [[nodiscard]] const wchar_t* GetLevelString(abstractions::LogLevel level) const noexcept;
        void FormatTimestamp(wchar_t* buffer, size_t bufferSize) const noexcept;
        [[nodiscard]] bool EnsureFileOpen();
        [[nodiscard]] bool EnsureDirectoryExists();
        [[nodiscard]] bool ShouldFlushImmediately(abstractions::LogLevel level) const noexcept;

        UniqueHandle mHFile;
        std::mutex mMutex;
        std::wstring mBuffer;
        std::wstring mLogFilePath;

        static constexpr size_t kBufferSize = 16384;
        static constexpr size_t kFlushThreshold = 8192;
    };

} // namespace winsetup::adapters::platform
