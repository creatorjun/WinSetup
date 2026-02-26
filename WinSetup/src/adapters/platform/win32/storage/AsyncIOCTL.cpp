#include "AsyncIOCTL.h"
#include <adapters/platform/win32/core/Win32HandleFactory.h>
#include <adapters/platform/win32/core/Win32ErrorHandler.h>
#include <algorithm>
#undef min
#undef max

namespace winsetup::adapters::platform {

    AsyncIOCTL::AsyncIOCTL() {
        mOperations.reserve(kMaxConcurrentOperations);
        mIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        HANDLE hThread = CreateThread(nullptr, 0, CompletionThreadProc, this, 0, nullptr);
        if (hThread)
            mCompletionThread = Win32HandleFactory::MakeHandle(hThread);
    }

    AsyncIOCTL::~AsyncIOCTL() {
        mShutdown.store(true);
        if (mIOCP)
            PostQueuedCompletionStatus(mIOCP, 0, kShutdownKey, nullptr);
        if (mCompletionThread)
            WaitForSingleObject(Win32HandleFactory::ToWin32Handle(mCompletionThread), INFINITE);
        if (mIOCP) {
            CloseHandle(mIOCP);
            mIOCP = nullptr;
        }
    }

    domain::Expected<uint32_t> AsyncIOCTL::SendAsync(
        HANDLE             hDevice,
        DWORD              ioControlCode,
        const void* inputBuffer,
        DWORD              inputBufferSize,
        DWORD              outputBufferSize,
        AsyncIOCTLCallback callback
    ) {
        if (mPendingOperations.load() >= mMaxConcurrentOps)
            return domain::Error{ L"Too many concurrent operations",
                ERROR_TOO_MANY_CMDS, domain::ErrorCategory::System };

        auto op = std::make_shared<AsyncOperation>();
        op->id = mNextOperationId.fetch_add(1);
        op->hDevice = hDevice;
        op->ioControlCode = ioControlCode;
        op->callback = callback;

        if (inputBuffer && inputBufferSize > 0) {
            op->inputBuffer.resize(inputBufferSize);
            std::memcpy(op->inputBuffer.data(), inputBuffer, inputBufferSize);
        }
        if (outputBufferSize > 0)
            op->outputBuffer.resize(outputBufferSize);

        HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!hEvent)
            return domain::Error{ L"Failed to create event",
                GetLastError(), domain::ErrorCategory::System };

        op->hEvent = Win32HandleFactory::MakeHandle(hEvent);
        ZeroMemory(&op->overlapped, sizeof(OVERLAPPED));

        if (!CreateIoCompletionPort(hDevice, mIOCP, kOperationKey, 0))
            return domain::Error{ L"Failed to associate device with IOCP",
                GetLastError(), domain::ErrorCategory::System };

        const uint32_t returnId = op->id;

        {
            std::lock_guard<std::mutex> lock(mOperationsMutex);
            mOperations.push_back(op);
        }
        mPendingOperations.fetch_add(1);

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            op->hDevice,
            op->ioControlCode,
            op->inputBuffer.empty() ? nullptr : op->inputBuffer.data(),
            static_cast<DWORD>(op->inputBuffer.size()),
            op->outputBuffer.empty() ? nullptr : op->outputBuffer.data(),
            static_cast<DWORD>(op->outputBuffer.size()),
            &bytesReturned,
            &op->overlapped
        );

        if (!result) {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                op->state.store(AsyncIOCTLState::Failed);
                op->errorCode = error;
                SetEvent(Win32HandleFactory::ToWin32Handle(op->hEvent));
                NotifyCompletion(op);
                mPendingOperations.fetch_sub(1);
                RemoveOperation(returnId);
                return domain::Error{ L"DeviceIoControl failed",
                    error, domain::ErrorCategory::System };
            }
            // ERROR_IO_PENDING: CompletionLoop가 완료 패킷을 처리
        }
        else {
            // 동기 완료: IOCP에 완료 패킷이 자동으로 큐잉됨
            // CompletionLoop가 패킷을 수신하여 처리하므로 여기서 별도 처리 불필요
        }

        return returnId;
    }

    DWORD WINAPI AsyncIOCTL::CompletionThreadProc(LPVOID lpParam) {
        static_cast<AsyncIOCTL*>(lpParam)->CompletionLoop();
        return 0;
    }

    void AsyncIOCTL::CompletionLoop() {
        while (true) {
            DWORD        bytesTransferred = 0;
            ULONG_PTR    completionKey = 0;
            LPOVERLAPPED pOverlapped = nullptr;

            BOOL ok = GetQueuedCompletionStatus(
                mIOCP, &bytesTransferred, &completionKey, &pOverlapped, INFINITE);

            // IOCP 자체 오류 (pOverlapped == nullptr, ok == FALSE)
            if (!ok && pOverlapped == nullptr)
                break;

            // 명시적 종료 패킷
            if (completionKey == kShutdownKey)
                break;

            if (!pOverlapped)
                continue;

            std::shared_ptr<AsyncOperation> op;
            {
                std::lock_guard<std::mutex> lock(mOperationsMutex);
                auto it = std::find_if(mOperations.begin(), mOperations.end(),
                    [pOverlapped](const auto& o) {
                        return o && &o->overlapped == pOverlapped;
                    });
                if (it != mOperations.end())
                    op = *it;
            }

            // 이미 CancelAll로 제거된 op — 카운터 보정 없이 skip
            if (!op)
                continue;

            if (ok) {
                op->bytesTransferred = bytesTransferred;
                op->errorCode = ERROR_SUCCESS;
                op->state.store(AsyncIOCTLState::Completed);
            }
            else {
                op->errorCode = GetLastError();
                const auto currentState = op->state.load();
                op->state.store(
                    currentState == AsyncIOCTLState::Cancelled
                    ? AsyncIOCTLState::Cancelled
                    : AsyncIOCTLState::Failed
                );
            }

            SetEvent(Win32HandleFactory::ToWin32Handle(op->hEvent));
            NotifyCompletion(op);
            mPendingOperations.fetch_sub(1);
            RemoveOperation(op->id);
        }
    }

    void AsyncIOCTL::NotifyCompletion(std::shared_ptr<AsyncOperation> op) {
        if (!op || !op->callback) return;
        AsyncIOCTLResult result{};
        result.bytesTransferred = op->bytesTransferred;
        result.errorCode = op->errorCode;
        result.state = op->state.load();
        result.outputBuffer = op->outputBuffer;
        op->callback(result);
    }

    domain::Expected<AsyncIOCTLResult> AsyncIOCTL::Wait(
        uint32_t operationId, DWORD timeoutMs
    ) {
        auto op = FindOperation(operationId);
        if (!op)
            return domain::Error{ L"Operation not found",
                ERROR_NOT_FOUND, domain::ErrorCategory::System };
        if (!op->hEvent)
            return domain::Error{ L"Invalid operation event",
                ERROR_INVALID_HANDLE, domain::ErrorCategory::System };

        DWORD waitResult = WaitForSingleObject(
            Win32HandleFactory::ToWin32Handle(op->hEvent), timeoutMs);

        if (waitResult == WAIT_TIMEOUT)
            return domain::Error{ L"Operation timeout",
                ERROR_TIMEOUT, domain::ErrorCategory::System };
        if (waitResult != WAIT_OBJECT_0)
            return domain::Error{ L"Wait failed",
                GetLastError(), domain::ErrorCategory::System };

        AsyncIOCTLResult result{};
        result.bytesTransferred = op->bytesTransferred;
        result.errorCode = op->errorCode;
        result.state = op->state.load();
        result.outputBuffer = op->outputBuffer;
        return result;
    }

    domain::Expected<std::vector<AsyncIOCTLResult>> AsyncIOCTL::WaitAll(
        const std::vector<uint32_t>& operationIds,
        DWORD                        timeoutMs
    ) {
        if (operationIds.empty())
            return std::vector<AsyncIOCTLResult>{};

        std::vector<HANDLE>                          events;
        std::vector<std::shared_ptr<AsyncOperation>> operations;
        events.reserve(operationIds.size());
        operations.reserve(operationIds.size());

        for (uint32_t opId : operationIds) {
            auto op = FindOperation(opId);
            if (op && op->hEvent) {
                events.push_back(Win32HandleFactory::ToWin32Handle(op->hEvent));
                operations.push_back(op);
            }
        }
        if (events.empty())
            return domain::Error{ L"No valid operations found",
                ERROR_NOT_FOUND, domain::ErrorCategory::System };

        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()), events.data(), TRUE, timeoutMs);

        if (waitResult == WAIT_TIMEOUT)
            return domain::Error{ L"Operations timeout",
                ERROR_TIMEOUT, domain::ErrorCategory::System };
        if (waitResult < WAIT_OBJECT_0 ||
            waitResult >= WAIT_OBJECT_0 + static_cast<DWORD>(events.size()))
            return domain::Error{ L"Wait failed",
                GetLastError(), domain::ErrorCategory::System };

        std::vector<AsyncIOCTLResult> results;
        results.reserve(operations.size());
        for (auto& op : operations) {
            AsyncIOCTLResult result{};
            result.bytesTransferred = op->bytesTransferred;
            result.errorCode = op->errorCode;
            result.state = op->state.load();
            result.outputBuffer = op->outputBuffer;
            results.push_back(std::move(result));
        }
        return results;
    }

    domain::Expected<void> AsyncIOCTL::Cancel(uint32_t operationId) {
        auto op = FindOperation(operationId);
        if (!op)
            return domain::Error{ L"Operation not found",
                ERROR_NOT_FOUND, domain::ErrorCategory::System };

        if (!CancelIoEx(op->hDevice, &op->overlapped)) {
            DWORD error = GetLastError();
            if (error != ERROR_NOT_FOUND)
                return domain::Error{ L"Failed to cancel operation",
                    error, domain::ErrorCategory::System };
        }
        op->state.store(AsyncIOCTLState::Cancelled);
        SetEvent(Win32HandleFactory::ToWin32Handle(op->hEvent));
        return domain::Expected<void>();
    }

    void AsyncIOCTL::CancelAll() {
        std::vector<std::shared_ptr<AsyncOperation>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mOperationsMutex);
            snapshot = mOperations;
            mOperations.clear();
        }

        for (auto& op : snapshot) {
            if (!op) continue;
            if (op->state.load() == AsyncIOCTLState::Pending) {
                CancelIoEx(op->hDevice, &op->overlapped);
                op->state.store(AsyncIOCTLState::Cancelled);
                if (op->hEvent)
                    SetEvent(Win32HandleFactory::ToWin32Handle(op->hEvent));
                mPendingOperations.fetch_sub(1);
            }
        }
    }

    bool AsyncIOCTL::IsOperationPending(uint32_t operationId) const {
        std::lock_guard<std::mutex> lock(mOperationsMutex);
        auto it = std::find_if(mOperations.begin(), mOperations.end(),
            [operationId](const auto& op) { return op && op->id == operationId; });
        return (it != mOperations.end()) &&
            (*it)->state.load() == AsyncIOCTLState::Pending;
    }

    std::shared_ptr<AsyncIOCTL::AsyncOperation> AsyncIOCTL::FindOperation(
        uint32_t operationId
    ) {
        std::lock_guard<std::mutex> lock(mOperationsMutex);
        auto it = std::find_if(mOperations.begin(), mOperations.end(),
            [operationId](const auto& op) { return op && op->id == operationId; });
        return (it != mOperations.end()) ? *it : nullptr;
    }

    void AsyncIOCTL::RemoveOperation(uint32_t operationId) {
        std::lock_guard<std::mutex> lock(mOperationsMutex);
        auto it = std::remove_if(mOperations.begin(), mOperations.end(),
            [operationId](const auto& op) { return op && op->id == operationId; });
        mOperations.erase(it, mOperations.end());
    }

    domain::Expected<std::vector<AsyncIOCTLResult>> AsyncIOCTLBatch::ExecuteAll(
        DWORD timeoutMs
    ) {
        mOperationIds.clear();
        mOperationIds.reserve(mOperations.size());
        for (const auto& op : mOperations) {
            auto result = mAsyncIO.SendAsync(
                op.hDevice, op.ioControlCode,
                op.inputData.empty() ? nullptr : op.inputData.data(),
                static_cast<DWORD>(op.inputData.size()),
                op.outputBufferSize, nullptr);
            if (result.HasValue())
                mOperationIds.push_back(result.Value());
            else
                return result.GetError();
        }
        return mAsyncIO.WaitAll(mOperationIds, timeoutMs);
    }

} // namespace winsetup::adapters::platform
