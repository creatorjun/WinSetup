#pragma once
#include <domain/primitives/Expected.h>
#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <Windows.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace winsetup::adapters::platform {

    enum class AsyncIOCTLState {
        Pending,
        Completed,
        Failed,
        Cancelled
    };

    struct AsyncIOCTLResult {
        DWORD             bytesTransferred = 0;
        DWORD             errorCode = 0;
        AsyncIOCTLState   state = AsyncIOCTLState::Pending;
        std::vector<BYTE> outputBuffer;

        [[nodiscard]] bool IsCompleted() const noexcept { return state == AsyncIOCTLState::Completed; }
        [[nodiscard]] bool IsFailed()    const noexcept { return state == AsyncIOCTLState::Failed; }
        [[nodiscard]] bool IsCancelled() const noexcept { return state == AsyncIOCTLState::Cancelled; }
    };

    using AsyncIOCTLCallback = std::function<void(const AsyncIOCTLResult&)>;

    class AsyncIOCTL {
    public:
        AsyncIOCTL();
        ~AsyncIOCTL();

        AsyncIOCTL(const AsyncIOCTL&) = delete;
        AsyncIOCTL& operator=(const AsyncIOCTL&) = delete;

        [[nodiscard]] domain::Expected<uint32_t> SendAsync(
            HANDLE             hDevice,
            DWORD              ioControlCode,
            const void* inputBuffer,
            DWORD              inputBufferSize,
            DWORD              outputBufferSize,
            AsyncIOCTLCallback callback
        );

        [[nodiscard]] domain::Expected<AsyncIOCTLResult> Wait(
            uint32_t operationId,
            DWORD    timeoutMs = INFINITE
        );

        [[nodiscard]] domain::Expected<std::vector<AsyncIOCTLResult>> WaitAll(
            const std::vector<uint32_t>& operationIds,
            DWORD                        timeoutMs = INFINITE
        );

        [[nodiscard]] domain::Expected<void> Cancel(uint32_t operationId);
        void CancelAll();

        [[nodiscard]] bool   IsOperationPending(uint32_t operationId) const;
        [[nodiscard]] size_t GetPendingOperationCount() const noexcept { return mPendingOperations.load(); }
        void SetMaxConcurrentOperations(size_t maxOps) noexcept { mMaxConcurrentOps = maxOps; }

    private:
        struct AsyncOperation {
            uint32_t                     id = 0;
            HANDLE                       hDevice = nullptr;
            DWORD                        ioControlCode = 0;
            std::vector<BYTE>            inputBuffer;
            std::vector<BYTE>            outputBuffer;
            OVERLAPPED                   overlapped{};
            UniqueHandle                 hEvent;
            AsyncIOCTLCallback           callback;
            std::atomic<AsyncIOCTLState> state{ AsyncIOCTLState::Pending };
            DWORD                        bytesTransferred = 0;
            DWORD                        errorCode = 0;

            AsyncOperation() = default;
        };

        void CompletionLoop();
        void NotifyCompletion(std::shared_ptr<AsyncOperation> op);
        [[nodiscard]] std::shared_ptr<AsyncOperation> FindOperation(uint32_t operationId);
        void RemoveOperation(uint32_t operationId);

        HANDLE                                       mIOCP = nullptr;
        UniqueHandle                                 mCompletionThread;
        std::atomic<uint32_t>                        mNextOperationId{ 1 };
        std::atomic<size_t>                          mPendingOperations{ 0 };
        size_t                                       mMaxConcurrentOps{ kMaxConcurrentOperations };
        std::vector<std::shared_ptr<AsyncOperation>> mOperations;
        mutable std::mutex                           mOperationsMutex;
        std::atomic<bool>                            mShutdown{ false };

        static constexpr size_t   kMaxConcurrentOperations = 32;
        static constexpr ULONG_PTR kShutdownKey = 0;
        static constexpr ULONG_PTR kOperationKey = 1;

        static DWORD WINAPI CompletionThreadProc(LPVOID lpParam);
    };

    class AsyncIOCTLBatch {
    public:
        explicit AsyncIOCTLBatch(AsyncIOCTL& asyncIO) : mAsyncIO(asyncIO) {}

        void AddOperation(
            HANDLE      hDevice,
            DWORD       ioControlCode,
            const void* inputBuffer,
            DWORD       inputBufferSize,
            DWORD       outputBufferSize
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

        [[nodiscard]] domain::Expected<std::vector<AsyncIOCTLResult>> ExecuteAll(
            DWORD timeoutMs = INFINITE
        );

        [[nodiscard]] size_t GetOperationCount() const noexcept { return mOperations.size(); }
        void Clear() { mOperations.clear(); mOperationIds.clear(); }

    private:
        struct Operation {
            HANDLE            hDevice = nullptr;
            DWORD             ioControlCode = 0;
            DWORD             inputBufferSize = 0;
            DWORD             outputBufferSize = 0;
            std::vector<BYTE> inputData;
        };

        AsyncIOCTL& mAsyncIO;
        std::vector<Operation>  mOperations;
        std::vector<uint32_t>   mOperationIds;
    };

} // namespace winsetup::adapters::platform
