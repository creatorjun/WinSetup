// src/adapters/platform/win32/concurrency/Win32ThreadPoolExecutor.cpp
#include <adapters/platform/win32/concurrency/Win32ThreadPoolExecutor.h>
#include <adapters/platform/win32/core/Win32HandleFactory.h>
#include <Windows.h>
#undef min
#undef max
#include <algorithm>
#include <thread>

namespace winsetup::adapters::platform {

    Win32ThreadPoolExecutor::Win32ThreadPoolExecutor(size_t threadCount) {
        if (threadCount == 0) {
            threadCount = static_cast<size_t>(std::thread::hardware_concurrency());
            if (threadCount == 0) threadCount = kDefaultThreadCount;
        }
        threadCount = std::min(threadCount, kMaxThreadCount);

        mWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        mThreads.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            HANDLE hThread = CreateThread(nullptr, 0, WorkerThreadProc, this, 0, nullptr);
            if (hThread)
                mThreads.push_back(Win32HandleFactory::MakeHandle(hThread));
        }
    }

    Win32ThreadPoolExecutor::~Win32ThreadPoolExecutor() {
        mShutdown.store(true);
        if (mWakeEvent)
            SetEvent(mWakeEvent);

        for (auto& t : mThreads)
            WaitForSingleObject(Win32HandleFactory::ToWin32Handle(t), INFINITE);

        if (mWakeEvent) {
            CloseHandle(mWakeEvent);
            mWakeEvent = nullptr;
        }
    }

    void Win32ThreadPoolExecutor::Post(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mQueueMutex);
            mTaskQueue.push(std::move(task));
        }
        if (mWakeEvent)
            SetEvent(mWakeEvent);
    }

    DWORD WINAPI Win32ThreadPoolExecutor::WorkerThreadProc(LPVOID lpParam) {
        static_cast<Win32ThreadPoolExecutor*>(lpParam)->WorkerLoop();
        return 0;
    }

    void Win32ThreadPoolExecutor::WorkerLoop() {
        while (true) {
            WaitForSingleObject(mWakeEvent, INFINITE);

            while (true) {
                std::function<void()> task;
                {
                    std::lock_guard<std::mutex> lock(mQueueMutex);
                    if (mTaskQueue.empty()) break;
                    task = std::move(mTaskQueue.front());
                    mTaskQueue.pop();
                    if (!mTaskQueue.empty())
                        SetEvent(mWakeEvent);
                }
                task();
            }

            if (mShutdown.load()) break;
        }
    }

} // namespace winsetup::adapters::platform
