// src/adapters/platform/win32/concurrency/Win32ThreadPoolExecutor.h
#pragma once
#include <abstractions/infrastructure/async/IExecutor.h>
#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <Windows.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

namespace winsetup::adapters::platform {

    class Win32ThreadPoolExecutor final : public abstractions::IExecutor {
    public:
        explicit Win32ThreadPoolExecutor(size_t threadCount = 0);
        ~Win32ThreadPoolExecutor() override;

        Win32ThreadPoolExecutor(const Win32ThreadPoolExecutor&) = delete;
        Win32ThreadPoolExecutor& operator=(const Win32ThreadPoolExecutor&) = delete;

        void Post(std::function<void()> task) override;

    private:
        static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
        void WorkerLoop();

        std::vector<UniqueHandle>           mThreads;
        std::queue<std::function<void()>>   mTaskQueue;
        std::mutex                          mQueueMutex;
        HANDLE                              mWakeEvent = nullptr;
        std::atomic<bool>                   mShutdown{ false };

        static constexpr size_t kDefaultThreadCount = 4;
        static constexpr size_t kMaxThreadCount = 16;
    };

} // namespace winsetup::adapters::platform
