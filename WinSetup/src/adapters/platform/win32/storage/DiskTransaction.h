// src/adapters/platform/win32/storage/DiskTransaction.h
#pragma once

#include "../../../../abstractions/services/storage/IDiskService.h"
#include "../../../../domain/primitives/Expected.h"
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <Windows.h>

namespace winsetup::adapters::platform {

    enum class TransactionState {
        NotStarted,
        Active,
        Committed,
        RolledBack,
        Failed
    };

    struct TransactionStep {
        std::wstring description;
        std::function<domain::Expected<void>()> execute;
        std::function<domain::Expected<void>()> rollback;
        bool executed = false;
        std::chrono::system_clock::time_point timestamp;

        TransactionStep(
            std::wstring desc,
            std::function<domain::Expected<void>()> exec,
            std::function<domain::Expected<void>()> roll
        )
            : description(std::move(desc))
            , execute(std::move(exec))
            , rollback(std::move(roll))
            , executed(false)
            , timestamp(std::chrono::system_clock::now())
        {
        }
    };

    class DiskTransaction {
    public:
        explicit DiskTransaction(
            uint32_t diskIndex,
            std::shared_ptr<abstractions::IDiskService> diskService
        );

        ~DiskTransaction();

        DiskTransaction(const DiskTransaction&) = delete;
        DiskTransaction& operator=(const DiskTransaction&) = delete;

        domain::Expected<void> Begin();

        domain::Expected<void> Commit();

        domain::Expected<void> Rollback();

        [[nodiscard]] bool IsActive() const noexcept {
            return mState == TransactionState::Active;
        }

        [[nodiscard]] domain::Expected<void> Execute(
            std::function<domain::Expected<void>()> operation
        );

        void AddStep(
            const std::wstring& description,
            std::function<domain::Expected<void>()> execute,
            std::function<domain::Expected<void>()> rollback
        );

        void AddCleanDiskStep();

        void AddCreatePartitionLayoutStep(const abstractions::PartitionLayout& layout);

        void AddFormatPartitionStep(
            uint32_t partitionIndex,
            domain::FileSystemType fileSystem,
            bool quickFormat = true
        );

        [[nodiscard]] TransactionState GetState() const noexcept {
            return mState;
        }

        [[nodiscard]] const std::vector<TransactionStep>& GetSteps() const noexcept {
            return mSteps;
        }

        [[nodiscard]] size_t GetExecutedStepCount() const noexcept;

        [[nodiscard]] std::wstring GetTransactionLog() const;

        void SetAutoRollback(bool enabled) noexcept {
            mAutoRollback = enabled;
        }

        void SetTimeout(uint32_t timeoutMs) noexcept {
            mTimeoutMs = timeoutMs;
        }

    private:
        [[nodiscard]] domain::Expected<void> BackupCurrentLayout();

        [[nodiscard]] domain::Expected<void> RestoreBackupLayout();

        [[nodiscard]] domain::Expected<void> ExecuteSteps();

        [[nodiscard]] domain::Expected<void> RollbackSteps();

        void LogStep(const std::wstring& message);

        [[nodiscard]] bool CheckTimeout() const;

        uint32_t mDiskIndex;
        std::shared_ptr<abstractions::IDiskService> mDiskService;

        TransactionState mState;
        std::vector<TransactionStep> mSteps;
        abstractions::PartitionLayout mBackupLayout;
        bool mLayoutBackedUp;
        bool mAutoRollback;
        uint32_t mTimeoutMs;

        std::chrono::system_clock::time_point mStartTime;
        std::vector<std::wstring> mTransactionLog;

        static constexpr uint32_t DEFAULT_TIMEOUT_MS = 300000;
    };

    class DiskTransactionBuilder {
    public:
        explicit DiskTransactionBuilder(
            uint32_t diskIndex,
            std::shared_ptr<abstractions::IDiskService> diskService
        )
            : mTransaction(std::make_unique<DiskTransaction>(diskIndex, diskService))
        {
        }

        DiskTransactionBuilder& WithCleanDisk() {
            mTransaction->AddCleanDiskStep();
            return *this;
        }

        DiskTransactionBuilder& WithPartitionLayout(const abstractions::PartitionLayout& layout) {
            mTransaction->AddCreatePartitionLayoutStep(layout);
            return *this;
        }

        DiskTransactionBuilder& WithFormatPartition(
            uint32_t partitionIndex,
            domain::FileSystemType fileSystem,
            bool quickFormat = true
        ) {
            mTransaction->AddFormatPartitionStep(partitionIndex, fileSystem, quickFormat);
            return *this;
        }

        DiskTransactionBuilder& WithAutoRollback(bool enabled) {
            mTransaction->SetAutoRollback(enabled);
            return *this;
        }

        DiskTransactionBuilder& WithTimeout(uint32_t timeoutMs) {
            mTransaction->SetTimeout(timeoutMs);
            return *this;
        }

        [[nodiscard]] std::unique_ptr<DiskTransaction> Build() {
            return std::move(mTransaction);
        }

    private:
        std::unique_ptr<DiskTransaction> mTransaction;
    };

}
