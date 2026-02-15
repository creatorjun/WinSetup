// src/abstractions/infrastructure/transaction/ITransactionManager.h
#pragma once

#include "ITransaction.h"
#include <domain/primitives/Expected.h>
#include <memory>
#include <cstdint>

namespace winsetup::abstractions {

    class ITransactionManager {
    public:
        virtual ~ITransactionManager() = default;

        [[nodiscard]] virtual domain::Expected<std::unique_ptr<ITransaction>>
            CreateTransaction() = 0;

        [[nodiscard]] virtual domain::Expected<std::unique_ptr<ITransaction>>
            CreateDiskTransaction(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<std::unique_ptr<ITransaction>>
            CreateVolumeTransaction(const std::wstring& volumePath) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            BeginBatch() = 0;

        [[nodiscard]] virtual domain::Expected<void>
            CommitBatch() = 0;

        [[nodiscard]] virtual domain::Expected<void>
            RollbackBatch() = 0;

        [[nodiscard]] virtual size_t GetActiveTransactionCount() const noexcept = 0;
        [[nodiscard]] virtual size_t GetCompletedTransactionCount() const noexcept = 0;

        virtual void SetDefaultTimeout(uint32_t timeoutMs) noexcept = 0;
        virtual void SetDefaultAutoRollback(bool enabled) noexcept = 0;
    };

}
