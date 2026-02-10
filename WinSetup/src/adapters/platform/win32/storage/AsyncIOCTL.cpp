// src/adapters/platform/win32/storage/AsyncIOCTL.cpp
#include "AsyncIOCTL.h"
#include "../core/Win32HandleFactory.h"
#include "../core/Win32ErrorHandler.h"
#include <algorithm>
#include <thread>

#undef min
#undef max

namespace winsetup::adapters::platform {

    AsyncIOCTL::AsyncIOCTL()
        : mNextOperationId(1)
        , mPendingOperations(0)
        , mMaxConcurrentOps(MAX_CONCURRENT_OPERATIONS)
        , mShutdown(false)
    {
        mOperations.reserve(MAX_CONCURRENT_OPERATIONS);

        size_t workerCount = (std::min)(DEFAULT_WORKER_THREADS,
            static_cast<size_t>(std::thread::hardware_concurrency()));

        for (size_t i = 0; i < workerCount; ++i) {
            HANDLE hThread = CreateThread(
                nullptr,
                0,
                WorkerThreadProc,
                this,
                0,
                nullptr
            );

            if (hThread != nullptr) {
                mWorkerThreads.push_back(Win32HandleFactory::MakeHandle(hThread));
            }
        }
    }

    AsyncIOCTL::~AsyncIOCTL() {
        mShutdown.store(true);

        CancelAll();

        for (auto& thread : mWorkerThreads) {
            if (thread) {
                WaitForSingleObject(Win32HandleFactory::ToWin32Handle(thread), INFINITE);
            }
        }

        mWorkerThreads.clear();
    }

    domain::Expected<uint32_t> AsyncIOCTL::SendAsync(
        HANDLE hDevice,
        DWORD ioControlCode,
        const void* inputBuffer,
        DWORD inputBufferSize,
        DWORD outputBufferSize,
        AsyncIOCTLCallback callback
    ) {
        if (mPendingOperations.load() >= mMaxConcurrentOps) {
            return domain::Error{
                L"Too many concurrent operations",
                ERROR_TOO_MANY_CMDS,
                domain::ErrorCategory::System
            };
        }

        auto op = std::make_shared<AsyncOperation>();
        op->id = mNextOperationId.fetch_add(1);
        op->hDevice = hDevice;
        op->ioControlCode = ioControlCode;
        op->callback = callback;

        if (inputBuffer && inputBufferSize > 0) {
            op->inputBuffer.resize(inputBufferSize);
            std::memcpy(op->inputBuffer.data(), inputBuffer, inputBufferSize);
        }

        if (outputBufferSize > 0) {
            op->outputBuffer.resize(outputBufferSize);
        }

        HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (hEvent == nullptr) {
            return domain::Error{
                L"Failed to create event",
                GetLastError(),
                domain::ErrorCategory::System
            };
        }

        op->hEvent = Win32HandleFactory::MakeHandle(hEvent);

        ZeroMemory(&op->overlapped, sizeof(OVERLAPPED));
        op->overlapped.hEvent = Win32HandleFactory::ToWin32Handle(op->hEvent);

        {
            std::lock_guard<std::mutex> lock(mOperationsMutex);
            mOperations.push_back(op);
        }

        mPendingOperations.fetch_add(1);

        HANDLE hThread = CreateThread(
            nullptr,
            0,
            [](LPVOID lpParam) -> DWORD {
                auto* context = static_cast<std::pair<AsyncIOCTL*, std::shared_ptr<AsyncOperation>>*>(lpParam);
                context->first->ProcessOperation(context->second);
                delete context;
                return 0;
            },
            new std::pair<AsyncIOCTL*, std::shared_ptr<AsyncOperation>>(this, op),
            0,
            nullptr
        );

        if (hThread != nullptr) {
            CloseHandle(hThread);
        }

        return op->id;
    }

    void AsyncIOCTL::ProcessOperation(std::shared_ptr<AsyncOperation> op) {
        if (!op || mShutdown.load()) {
            return;
        }

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

            if (error == ERROR_IO_PENDING) {
                DWORD waitResult = WaitForSingleObject(op->overlapped.hEvent, INFINITE);

                if (waitResult == WAIT_OBJECT_0) {
                    if (GetOverlappedResult(op->hDevice, &op->overlapped, &bytesReturned, FALSE)) {
                        op->state.store(AsyncIOCTLState::Completed);
                        op->bytesTransferred = bytesReturned;
                        op->errorCode = ERROR_SUCCESS;
                    }
                    else {
                        op->state.store(AsyncIOCTLState::Failed);
                        op->errorCode = GetLastError();
                    }
                }
                else {
                    op->state.store(AsyncIOCTLState::Failed);
                    op->errorCode = GetLastError();
                }
            }
            else {
                op->state.store(AsyncIOCTLState::Failed);
                op->errorCode = error;
            }
        }
        else {
            op->state.store(AsyncIOCTLState::Completed);
            op->bytesTransferred = bytesReturned;
            op->errorCode = ERROR_SUCCESS;
        }

        NotifyCompletion(op);
        mPendingOperations.fetch_sub(1);
    }

    void AsyncIOCTL::NotifyCompletion(std::shared_ptr<AsyncOperation> op) {
        if (!op || !op->callback) {
            return;
        }

        AsyncIOCTLResult result{};
        result.bytesTransferred = op->bytesTransferred;
        result.errorCode = op->errorCode;
        result.state = op->state.load();
        result.outputBuffer = op->outputBuffer;

        op->callback(result);
    }

    domain::Expected<AsyncIOCTLResult> AsyncIOCTL::Wait(uint32_t operationId, DWORD timeoutMs) {
        auto op = FindOperation(operationId);
        if (!op) {
            return domain::Error{
                L"Operation not found",
                ERROR_NOT_FOUND,
                domain::ErrorCategory::System
            };
        }

        if (!op->hEvent) {
            return domain::Error{
                L"Invalid operation event",
                ERROR_INVALID_HANDLE,
                domain::ErrorCategory::System
            };
        }

        DWORD waitResult = WaitForSingleObject(
            Win32HandleFactory::ToWin32Handle(op->hEvent),
            timeoutMs
        );

        if (waitResult == WAIT_TIMEOUT) {
            return domain::Error{
                L"Operation timeout",
                ERROR_TIMEOUT,
                domain::ErrorCategory::System
            };
        }

        if (waitResult != WAIT_OBJECT_0) {
            return domain::Error{
                L"Wait failed",
                GetLastError(),
                domain::ErrorCategory::System
            };
        }

        AsyncIOCTLResult result{};
        result.bytesTransferred = op->bytesTransferred;
        result.errorCode = op->errorCode;
        result.state = op->state.load();
        result.outputBuffer = op->outputBuffer;

        RemoveOperation(operationId);

        return result;
    }

    domain::Expected<std::vector<AsyncIOCTLResult>> AsyncIOCTL::WaitAll(
        const std::vector<uint32_t>& operationIds,
        DWORD timeoutMs
    ) {
        if (operationIds.empty()) {
            return std::vector<AsyncIOCTLResult>{};
        }

        std::vector<HANDLE> events;
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

        if (events.empty()) {
            return domain::Error{
                L"No valid operations found",
                ERROR_NOT_FOUND,
                domain::ErrorCategory::System
            };
        }

        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()),
            events.data(),
            TRUE,
            timeoutMs
        );

        if (waitResult == WAIT_TIMEOUT) {
            return domain::Error{
                L"Operations timeout",
                ERROR_TIMEOUT,
                domain::ErrorCategory::System
            };
        }

        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + events.size()) {
            std::vector<AsyncIOCTLResult> results;
            results.reserve(operations.size());

            for (auto& op : operations) {
                AsyncIOCTLResult result{};
                result.bytesTransferred = op->bytesTransferred;
                result.errorCode = op->errorCode;
                result.state = op->state.load();
                result.outputBuffer = op->outputBuffer;
                results.push_back(std::move(result));

                RemoveOperation(op->id);
            }

            return results;
        }

        return domain::Error{
            L"Wait failed",
            GetLastError(),
            domain::ErrorCategory::System
        };
    }

    domain::Expected<void> AsyncIOCTL::Cancel(uint32_t operationId) {
        auto op = FindOperation(operationId);
        if (!op) {
            return domain::Error{
                L"Operation not found",
                ERROR_NOT_FOUND,
                domain::ErrorCategory::System
            };
        }

        if (!CancelIoEx(op->hDevice, &op->overlapped)) {
            DWORD error = GetLastError();
            if (error != ERROR_NOT_FOUND) {
                return domain::Error{
                    L"Failed to cancel operation",
                    error,
                    domain::ErrorCategory::System
                };
            }
        }

        op->state.store(AsyncIOCTLState::Cancelled);
        SetEvent(Win32HandleFactory::ToWin32Handle(op->hEvent));

        RemoveOperation(operationId);

        return domain::Expected<void>();
    }

    void AsyncIOCTL::CancelAll() {
        std::lock_guard<std::mutex> lock(mOperationsMutex);

        for (auto& op : mOperations) {
            if (op && op->state.load() == AsyncIOCTLState::Pending) {
                CancelIoEx(op->hDevice, &op->overlapped);
                op->state.store(AsyncIOCTLState::Cancelled);
                if (op->hEvent) {
                    SetEvent(Win32HandleFactory::ToWin32Handle(op->hEvent));
                }
            }
        }

        mOperations.clear();
        mPendingOperations.store(0);
    }

    bool AsyncIOCTL::IsOperationPending(uint32_t operationId) const {
        std::lock_guard<std::mutex> lock(mOperationsMutex);

        auto it = std::find_if(mOperations.begin(), mOperations.end(),
            [operationId](const auto& op) {
                return op && op->id == operationId;
            });

        return (it != mOperations.end()) && (*it)->state.load() == AsyncIOCTLState::Pending;
    }

    std::shared_ptr<AsyncIOCTL::AsyncOperation> AsyncIOCTL::FindOperation(uint32_t operationId) {
        std::lock_guard<std::mutex> lock(mOperationsMutex);

        auto it = std::find_if(mOperations.begin(), mOperations.end(),
            [operationId](const auto& op) {
                return op && op->id == operationId;
            });

        return (it != mOperations.end()) ? *it : nullptr;
    }

    void AsyncIOCTL::RemoveOperation(uint32_t operationId) {
        std::lock_guard<std::mutex> lock(mOperationsMutex);

        auto it = std::remove_if(mOperations.begin(), mOperations.end(),
            [operationId](const auto& op) {
                return op && op->id == operationId;
            });

        mOperations.erase(it, mOperations.end());
    }

    DWORD WINAPI AsyncIOCTL::WorkerThreadProc(LPVOID lpParam) {
        return 0;
    }

    domain::Expected<std::vector<AsyncIOCTLResult>> AsyncIOCTLBatch::ExecuteAll(DWORD timeoutMs) {
        mOperationIds.clear();
        mOperationIds.reserve(mOperations.size());

        for (const auto& op : mOperations) {
            auto result = mAsyncIO.SendAsync(
                op.hDevice,
                op.ioControlCode,
                op.inputData.empty() ? nullptr : op.inputData.data(),
                static_cast<DWORD>(op.inputData.size()),
                op.outputBufferSize,
                nullptr
            );

            if (result.HasValue()) {
                mOperationIds.push_back(result.Value());
            }
            else {
                return result.GetError();
            }
        }

        return mAsyncIO.WaitAll(mOperationIds, timeoutMs);
    }

}
