// src/adapters/platform/win32/storage/AsyncIOCTL.h
#pragma once

#include <domain/primitives/Expected.h>
#include "../memory/UniqueHandle.h"
#include <Windows.h>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

namespace winsetup::adapters::platform {

    enum class AsyncIOCTLState {
        Pending,
        Completed,
        Failed,
        Cancelled
    };

    struct AsyncIOCTLResult {
        DWORD bytesTransferred;
        DWORD errorCode;
        AsyncIOCTLState state;
        std::vector<BYTE> outputBuffer;

        [[nodiscard]] bool IsCompleted() const noexcept {
            return state == AsyncIOCTLState::Completed;
        }

        [[nodiscard]] bool IsFailed() const noexcept {
            return state == AsyncIOCTLState::Failed;
        }

        [[nodiscard]] bool IsCancelled() const noexcept {
            return state == AsyncIOCTLState::Cancelled;
        }
    };

    using AsyncIOCTLCallback = std::function<void(const AsyncIOCTLResult&)>;

    class AsyncIOCTL {
    public:
        AsyncIOCTL();
        ~AsyncIOCTL();

        AsyncIOCTL(const AsyncIOCTL&) = delete;
        AsyncIOCTL& operator=(const AsyncIOCTL&) = delete;

        [[nodiscard]] domain::Expected<uint32_t> SendAsync(
            HANDLE hDevice,
            DWORD ioControlCode,
            const void* inputBuffer,
            DWORD inputBufferSize,
            DWORD outputBufferSize,
            AsyncIOCTLCallback callback
        );

        [[nodiscard]] domain::Expected<AsyncIOCTLResult> Wait(uint32_t operationId, DWORD timeoutMs = INFINITE);

        [[nodiscard]] domain::Expected<std::vector<AsyncIOCTLResult>> WaitAll(
            const std::vector<uint32_t>& operationIds,
            DWORD timeoutMs = INFINITE
        );

        [[nodiscard]] domain::Expected<void> Cancel(uint32_t operationId);

        void CancelAll();

        [[nodiscard]] bool IsOperationPending(uint32_t operationId) const;

        [[nodiscard]] size_t GetPendingOperationCount() const noexcept {
            return mPendingOperations.load();
        }

        void SetMaxConcurrentOperations(size_t maxOps) noexcept {
            mMaxConcurrentOps = maxOps;
        }

    private:
        struct AsyncOperation {
            uint32_t id;
            HANDLE hDevice;
            DWORD ioControlCode;
            std::vector<BYTE> inputBuffer;
            std::vector<BYTE> outputBuffer;
            OVERLAPPED overlapped;
            UniqueHandle hEvent;
            AsyncIOCTLCallback callback;
            std::atomic<AsyncIOCTLState> state;
            DWORD bytesTransferred;
            DWORD errorCode;

            AsyncOperation()
                : id(0)
                , hDevice(nullptr)
                , ioControlCode(0)
                , overlapped{}
                , state(AsyncIOCTLState::Pending)
                , bytesTransferred(0)
                , errorCode(0)
            {
            }
        };

        static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);

        void ProcessOperation(std::shared_ptr<AsyncOperation> op);

        void NotifyCompletion(std::shared_ptr<AsyncOperation> op);

        [[nodiscard]] std::shared_ptr<AsyncOperation> FindOperation(uint32_t operationId);

        void RemoveOperation(uint32_t operationId);

        std::atomic<uint32_t> mNextOperationId;
        std::atomic<size_t> mPendingOperations;
        size_t mMaxConcurrentOps;

        std::vector<std::shared_ptr<AsyncOperation>> mOperations;
        mutable std::mutex mOperationsMutex;

        std::vector<UniqueHandle> mWorkerThreads;
        std::atomic<bool> mShutdown;

        static constexpr size_t DEFAULT_WORKER_THREADS = 4;
        static constexpr size_t MAX_CONCURRENT_OPERATIONS = 32;
    };

    class AsyncIOCTLBatch {
    public:
        explicit AsyncIOCTLBatch(AsyncIOCTL& asyncIO)
            : mAsyncIO(asyncIO)
        {
        }

        void AddOperation(
            HANDLE hDevice,
            DWORD ioControlCode,
            const void* inputBuffer,
            DWORD inputBufferSize,
            DWORD outputBufferSize
        ) {
            Operation op{};
            op.hDevice = hDevice;
            op.ioControlCode = ioControlCode;
            op.inputBufferSize = inputBufferSize;
            op.outputBufferSize = outputBufferSize;

            if (inputBuffer && inputBufferSize > 0) {
                op.inputData.resize(inputBufferSize);
                std::memcpy(op.inputData.data(), inputBuffer, inputBufferSize);
            }

            mOperations.push_back(std::move(op));
        }

        [[nodiscard]] domain::Expected<std::vector<AsyncIOCTLResult>> ExecuteAll(DWORD timeoutMs = INFINITE);

        [[nodiscard]] size_t GetOperationCount() const noexcept {
            return mOperations.size();
        }

        void Clear() {
            mOperations.clear();
            mOperationIds.clear();
        }

    private:
        struct Operation {
            HANDLE hDevice;
            DWORD ioControlCode;
            DWORD inputBufferSize;
            DWORD outputBufferSize;
            std::vector<BYTE> inputData;
        };

        AsyncIOCTL& mAsyncIO;
        std::vector<Operation> mOperations;
        std::vector<uint32_t> mOperationIds;
    };

}
