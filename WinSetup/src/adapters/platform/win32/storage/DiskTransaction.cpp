// src/adapters/platform/win32/storage/DiskTransaction.cpp
#include "DiskTransaction.h"
#include <algorithm>
#include <sstream>

#ifndef ERROR_INVALID_STATE
#define ERROR_INVALID_STATE 5023L
#endif

#ifndef ERROR_TIMEOUT
#define ERROR_TIMEOUT 1460L
#endif

#undef GetMessage

namespace winsetup::adapters::platform {

    DiskTransaction::DiskTransaction(
        uint32_t diskIndex,
        std::shared_ptr<abstractions::IDiskService> diskService
    )
        : mDiskIndex(diskIndex)
        , mDiskService(std::move(diskService))
        , mState(TransactionState::NotStarted)
        , mLayoutBackedUp(false)
        , mAutoRollback(true)
        , mTimeoutMs(DEFAULT_TIMEOUT_MS)
    {
        mSteps.reserve(16);
        mTransactionLog.reserve(32);
    }

    DiskTransaction::~DiskTransaction() {
        if (mState == TransactionState::Active && mAutoRollback) {
            Rollback();
        }
    }

    domain::Expected<void> DiskTransaction::Begin() {
        if (mState != TransactionState::NotStarted) {
            return domain::Error{
                L"Transaction already started",
                ERROR_ALREADY_INITIALIZED,
                domain::ErrorCategory::Disk
            };
        }

        mStartTime = std::chrono::system_clock::now();
        LogStep(L"Transaction started for disk " + std::to_wstring(mDiskIndex));

        auto backupResult = BackupCurrentLayout();
        if (!backupResult.HasValue()) {
            mState = TransactionState::Failed;
            const auto& error = backupResult.GetError();
            LogStep(L"Failed to backup current layout: " + error.GetMessage());
            return backupResult;
        }

        mState = TransactionState::Active;
        LogStep(L"Layout backed up successfully");

        return domain::Expected<void>();
    }

    domain::Expected<void> DiskTransaction::Commit() {
        if (mState != TransactionState::Active) {
            return domain::Error{
                L"Transaction is not active",
                ERROR_INVALID_STATE,
                domain::ErrorCategory::Disk
            };
        }

        LogStep(L"Committing transaction...");

        mState = TransactionState::Committed;
        mLayoutBackedUp = false;

        LogStep(L"Transaction committed successfully");

        return domain::Expected<void>();
    }

    domain::Expected<void> DiskTransaction::Rollback() {
        if (mState != TransactionState::Active && mState != TransactionState::Failed) {
            return domain::Error{
                L"Transaction cannot be rolled back in current state",
                ERROR_INVALID_STATE,
                domain::ErrorCategory::Disk
            };
        }

        LogStep(L"Rolling back transaction...");

        auto rollbackResult = RollbackSteps();
        if (!rollbackResult.HasValue()) {
            const auto& error = rollbackResult.GetError();
            LogStep(L"Rollback failed: " + error.GetMessage());
            return rollbackResult;
        }

        if (mLayoutBackedUp) {
            auto restoreResult = RestoreBackupLayout();
            if (!restoreResult.HasValue()) {
                const auto& error = restoreResult.GetError();
                LogStep(L"Failed to restore backup layout: " + error.GetMessage());
                return restoreResult;
            }
        }

        mState = TransactionState::RolledBack;
        LogStep(L"Transaction rolled back successfully");

        return domain::Expected<void>();
    }

    domain::Expected<void> DiskTransaction::Execute(
        std::function<domain::Expected<void>()> operation
    ) {
        if (mState == TransactionState::NotStarted) {
            auto beginResult = Begin();
            if (!beginResult.HasValue()) {
                return beginResult;
            }
        }

        if (mState != TransactionState::Active) {
            return domain::Error{
                L"Transaction is not active",
                ERROR_INVALID_STATE,
                domain::ErrorCategory::Disk
            };
        }

        if (CheckTimeout()) {
            auto error = domain::Error{
                L"Transaction timeout exceeded",
                ERROR_TIMEOUT,
                domain::ErrorCategory::Disk
            };

            if (mAutoRollback) {
                Rollback();
            }

            return error;
        }

        auto result = ExecuteSteps();
        if (!result.HasValue()) {
            mState = TransactionState::Failed;
            const auto& error = result.GetError();
            LogStep(L"Execution failed: " + error.GetMessage());

            if (mAutoRollback) {
                Rollback();
            }

            return result;
        }

        if (operation) {
            auto opResult = operation();
            if (!opResult.HasValue()) {
                mState = TransactionState::Failed;
                const auto& error = opResult.GetError();
                LogStep(L"Operation failed: " + error.GetMessage());

                if (mAutoRollback) {
                    Rollback();
                }

                return opResult;
            }
        }

        return Commit();
    }

    void DiskTransaction::AddStep(
        const std::wstring& description,
        std::function<domain::Expected<void>()> execute,
        std::function<domain::Expected<void>()> rollback
    ) {
        mSteps.emplace_back(description, std::move(execute), std::move(rollback));
        LogStep(L"Added step: " + description);
    }

    void DiskTransaction::AddCleanDiskStep() {
        AddStep(
            L"Clean disk " + std::to_wstring(mDiskIndex),
            [this]() -> domain::Expected<void> {
                return mDiskService->CleanDisk(mDiskIndex);
            },
            [this]() -> domain::Expected<void> {
                return RestoreBackupLayout();
            }
        );
    }

    void DiskTransaction::AddCreatePartitionLayoutStep(const abstractions::PartitionLayout& layout) {
        AddStep(
            L"Create partition layout on disk " + std::to_wstring(mDiskIndex),
            [this, layout]() -> domain::Expected<void> {
                return mDiskService->CreatePartitionLayout(mDiskIndex, layout);
            },
            [this]() -> domain::Expected<void> {
                return RestoreBackupLayout();
            }
        );
    }

    void DiskTransaction::AddFormatPartitionStep(
        uint32_t partitionIndex,
        domain::FileSystemType fileSystem,
        bool quickFormat
    ) {
        std::wstring fsName = L"Unknown";
        switch (fileSystem) {
        case domain::FileSystemType::NTFS: fsName = L"NTFS"; break;
        case domain::FileSystemType::FAT32: fsName = L"FAT32"; break;
        case domain::FileSystemType::exFAT: fsName = L"exFAT"; break;
        case domain::FileSystemType::ReFS: fsName = L"ReFS"; break;
        default: break;
        }

        AddStep(
            L"Format partition " + std::to_wstring(partitionIndex) + L" as " + fsName,
            [this, partitionIndex, fileSystem, quickFormat]() -> domain::Expected<void> {
                return mDiskService->FormatPartition(mDiskIndex, partitionIndex, fileSystem, quickFormat);
            },
            [this]() -> domain::Expected<void> {
                return RestoreBackupLayout();
            }
        );
    }

    domain::Expected<void> DiskTransaction::BackupCurrentLayout() {
        auto layoutResult = mDiskService->GetCurrentLayout(mDiskIndex);
        if (!layoutResult.HasValue()) {
            return layoutResult.GetError();
        }

        mBackupLayout = layoutResult.Value();
        mLayoutBackedUp = true;

        return domain::Expected<void>();
    }

    domain::Expected<void> DiskTransaction::RestoreBackupLayout() {
        if (!mLayoutBackedUp) {
            return domain::Error{
                L"No backup layout available",
                ERROR_FILE_NOT_FOUND,
                domain::ErrorCategory::Disk
            };
        }

        LogStep(L"Restoring backup layout...");

        return mDiskService->RestoreLayout(mDiskIndex, mBackupLayout);
    }

    domain::Expected<void> DiskTransaction::ExecuteSteps() {
        for (auto& step : mSteps) {
            if (step.executed) {
                continue;
            }

            if (CheckTimeout()) {
                return domain::Error{
                    L"Transaction timeout during step: " + step.description,
                    ERROR_TIMEOUT,
                    domain::ErrorCategory::Disk
                };
            }

            LogStep(L"Executing: " + step.description);

            auto result = step.execute();
            if (!result.HasValue()) {
                LogStep(L"Step failed: " + step.description);
                return result;
            }

            step.executed = true;
            step.timestamp = std::chrono::system_clock::now();
            LogStep(L"Step completed: " + step.description);
        }

        return domain::Expected<void>();
    }

    domain::Expected<void> DiskTransaction::RollbackSteps() {
        bool anyFailed = false;
        domain::Error lastError{ L"Unknown error", 0, domain::ErrorCategory::Unknown };

        for (auto it = mSteps.rbegin(); it != mSteps.rend(); ++it) {
            if (!it->executed) {
                continue;
            }

            LogStep(L"Rolling back: " + it->description);

            if (it->rollback) {
                auto result = it->rollback();
                if (!result.HasValue()) {
                    LogStep(L"Rollback failed for: " + it->description);
                    anyFailed = true;
                    lastError = result.GetError();
                }
                else {
                    it->executed = false;
                    LogStep(L"Rollback completed: " + it->description);
                }
            }
        }

        if (anyFailed) {
            return lastError;
        }

        return domain::Expected<void>();
    }

    void DiskTransaction::LogStep(const std::wstring& message) {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - mStartTime
        ).count();

        std::wstring logEntry = L"[+" + std::to_wstring(duration) + L"ms] " + message;
        mTransactionLog.push_back(logEntry);
    }

    bool DiskTransaction::CheckTimeout() const {
        if (mTimeoutMs == 0) {
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - mStartTime
        ).count();

        return elapsed > mTimeoutMs;
    }

    size_t DiskTransaction::GetExecutedStepCount() const noexcept {
        return std::count_if(mSteps.begin(), mSteps.end(),
            [](const auto& step) { return step.executed; });
    }

    std::wstring DiskTransaction::GetTransactionLog() const {
        std::wstring log;
        log.reserve(mTransactionLog.size() * 100);

        log += L"=== Disk Transaction Log (Disk " + std::to_wstring(mDiskIndex) + L") ===\n";

        for (const auto& entry : mTransactionLog) {
            log += entry + L"\n";
        }

        log += L"\n=== Summary ===\n";
        log += L"Total Steps: " + std::to_wstring(mSteps.size()) + L"\n";
        log += L"Executed Steps: " + std::to_wstring(GetExecutedStepCount()) + L"\n";
        log += L"State: ";

        switch (mState) {
        case TransactionState::NotStarted: log += L"Not Started"; break;
        case TransactionState::Active: log += L"Active"; break;
        case TransactionState::Committed: log += L"Committed"; break;
        case TransactionState::RolledBack: log += L"Rolled Back"; break;
        case TransactionState::Failed: log += L"Failed"; break;
        default: log += L"Unknown"; break;
        }

        log += L"\n";

        return log;
    }

}
