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

        mIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0,
            static_cast<DWORD>(threadCount));

        mThreads.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            HANDLE hThread = CreateThread(nullptr, 0, WorkerThreadProc, this, 0, nullptr);
            if (hThread)
                mThreads.push_back(Win32HandleFactory::MakeHandle(hThread));
        }
    }

    Win32ThreadPoolExecutor::~Win32ThreadPoolExecutor() {
        mShutdown.store(true);

        for (size_t i = 0; i < mThreads.size(); ++i)
            PostQueuedCompletionStatus(mIOCP, 0, kShutdownKey, nullptr);

        for (auto& t : mThreads)
            WaitForSingleObject(Win32HandleFactory::ToWin32Handle(t), INFINITE);

        if (mIOCP) {
            CloseHandle(mIOCP);
            mIOCP = nullptr;
        }
    }

    void Win32ThreadPoolExecutor::Post(std::function<void()> task) {
        auto* raw = new std::function<void()>(std::move(task));
        if (!PostQueuedCompletionStatus(mIOCP, 0, kTaskKey,
            reinterpret_cast<LPOVERLAPPED>(raw))) {
            delete raw;
        }
    }

    DWORD WINAPI Win32ThreadPoolExecutor::WorkerThreadProc(LPVOID lpParam) {
        static_cast<Win32ThreadPoolExecutor*>(lpParam)->WorkerLoop();
        return 0;
    }

    void Win32ThreadPoolExecutor::WorkerLoop() {
        while (true) {
            DWORD       bytesTransferred = 0;
            ULONG_PTR   completionKey = 0;
            LPOVERLAPPED pOverlapped = nullptr;

            BOOL ok = GetQueuedCompletionStatus(mIOCP, &bytesTransferred,
                &completionKey, &pOverlapped, INFINITE);

            if (!ok || completionKey == kShutdownKey)
                break;

            if (completionKey == kTaskKey && pOverlapped) {
                auto* fn = reinterpret_cast<std::function<void()>*>(pOverlapped);
                (*fn)();
                delete fn;
            }
        }
    }

} // namespace winsetup::adapters::platform
