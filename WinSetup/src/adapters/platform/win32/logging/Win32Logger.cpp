#include "adapters/platform/win32/logging/Win32Logger.h"
#include "adapters/platform/win32/core/Win32HandleFactory.h"
#include <Windows.h>

namespace winsetup::adapters::platform {

    Win32Logger::Win32Logger(const std::wstring& logFilePath)
        : mLogFilePath(logFilePath)
    {
        mWriteBuffer.reserve(kWriteBufferReserve);
        mQueue.reserve(kQueueReserve);
        mFlushQueue.reserve(kQueueReserve);

        if (!InitializeCriticalSectionAndSpinCount(&mQueueLock, 1000))
            InitializeCriticalSection(&mQueueLock);
        InitializeConditionVariable(&mQueueCV);

        HANDLE hThread = CreateThread(nullptr, 0, WriterThreadProc, this, 0, nullptr);
        if (hThread)
            mWriterThread = Win32HandleFactory::MakeHandle(hThread);
    }


    Win32Logger::~Win32Logger() {
        Flush();

        EnterCriticalSection(&mQueueLock);
        mShutdown = true;
        LeaveCriticalSection(&mQueueLock);
        WakeConditionVariable(&mQueueCV);

        if (mWriterThread)
            WaitForSingleObject(Win32HandleFactory::ToWin32Handle(mWriterThread), INFINITE);

        DeleteCriticalSection(&mQueueLock);
    }

    void Win32Logger::Log(
        abstractions::LogLevel      level,
        const std::wstring& message,
        const std::source_location& /*location*/)
    {
        LogEntry entry{};
        entry.level = level;
        entry.message = message;
        FormatTimestamp(entry.timestamp, 32);

        OutputDebugStringW(entry.message.c_str());

        EnterCriticalSection(&mQueueLock);
        mQueue.push_back(std::move(entry));
        const bool shouldWakeImmediately =
            (level == abstractions::LogLevel::Error ||
                level == abstractions::LogLevel::Fatal);
        if (shouldWakeImmediately)
            mFlushRequested = true;
        LeaveCriticalSection(&mQueueLock);

        if (shouldWakeImmediately)
            WakeConditionVariable(&mQueueCV);
    }

    void Win32Logger::Flush() {
        EnterCriticalSection(&mQueueLock);
        mFlushRequested = true;
        LeaveCriticalSection(&mQueueLock);
        WakeConditionVariable(&mQueueCV);

        // Writer 스레드가 flush를 처리할 때까지 잠시 양보
        // (완전한 동기 flush가 필요하다면 flush 완료 이벤트 추가 가능)
        Sleep(0);
    }

    DWORD WINAPI Win32Logger::WriterThreadProc(LPVOID lpParam) {
        static_cast<Win32Logger*>(lpParam)->WriterLoop();
        return 0;
    }

    void Win32Logger::WriterLoop() {
        while (true) {
            EnterCriticalSection(&mQueueLock);

            while (mQueue.empty() && !mShutdown && !mFlushRequested)
                SleepConditionVariableCS(&mQueueCV, &mQueueLock, 100);

            mFlushQueue.clear();
            std::swap(mQueue, mFlushQueue);
            const bool shutdown = mShutdown;
            mFlushRequested = false;

            LeaveCriticalSection(&mQueueLock);

            if (!mFlushQueue.empty())
                FlushBatch(mFlushQueue);

            if (shutdown) {
                // 종료 직전 잔여 큐 한 번 더 드레인
                EnterCriticalSection(&mQueueLock);
                std::swap(mQueue, mFlushQueue);
                LeaveCriticalSection(&mQueueLock);
                if (!mFlushQueue.empty())
                    FlushBatch(mFlushQueue);
                break;
            }
        }
    }

    void Win32Logger::FlushBatch(std::vector<LogEntry>& batch) {
        if (!EnsureFileOpen()) return;

        mWriteBuffer.clear();
        for (const auto& entry : batch)
            FormatAndWrite(entry);

        if (mWriteBuffer.empty()) return;

        DWORD bytesWritten = 0;
        WriteFile(
            Win32HandleFactory::ToWin32Handle(mHFile),
            mWriteBuffer.data(),
            static_cast<DWORD>(mWriteBuffer.size() * sizeof(wchar_t)),
            &bytesWritten,
            nullptr);
        FlushFileBuffers(Win32HandleFactory::ToWin32Handle(mHFile));
    }

    void Win32Logger::FormatAndWrite(const LogEntry& entry) {
        mWriteBuffer += entry.timestamp;
        mWriteBuffer += L" [";
        mWriteBuffer += GetLevelString(entry.level);
        mWriteBuffer += L"] ";
        mWriteBuffer += entry.message;
        mWriteBuffer += L"\r\n";
    }

    bool Win32Logger::EnsureDirectoryExists() {
        const size_t lastSlash = mLogFilePath.find_last_of(L"\\/");
        if (lastSlash == std::wstring::npos)
            return true;

        const std::wstring dirPath = mLogFilePath.substr(0, lastSlash);
        const DWORD attribs = GetFileAttributesW(dirPath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
            return (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (!CreateDirectoryW(dirPath.c_str(), nullptr)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
                return false;
        }
        return true;
    }

    bool Win32Logger::EnsureFileOpen() {
        if (mHFile) return true;
        if (!EnsureDirectoryExists()) return false;

        HANDLE hFile = CreateFileW(
            mLogFilePath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE) return false;
        SetFilePointer(hFile, 0, nullptr, FILE_END);
        mHFile = Win32HandleFactory::MakeHandle(hFile);
        return true;
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
        swprintf_s(buffer, bufferSize,
            L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }

} // namespace winsetup::adapters::platform
