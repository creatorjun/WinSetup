// src/abstractions/infrastructure/transaction/ITransaction.h
#pragma once

#include <domain/primitives/Expected.h>
#include <string>
#include <functional>

namespace winsetup::abstractions {

    enum class TransactionState {
        NotStarted,
        Active,
        Committed,
        RolledBack,
        Failed
    };

    class ITransaction {
    public:
        virtual ~ITransaction() = default;

        [[nodiscard]] virtual domain::Expected<void> Begin() = 0;
        [[nodiscard]] virtual domain::Expected<void> Commit() = 0;
        [[nodiscard]] virtual domain::Expected<void> Rollback() = 0;

        [[nodiscard]] virtual bool IsActive() const noexcept = 0;
        [[nodiscard]] virtual TransactionState GetState() const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<void> Execute(
            std::function<domain::Expected<void>()> operation
        ) = 0;

        virtual void SetAutoRollback(bool enabled) noexcept = 0;
        virtual void SetTimeout(uint32_t timeoutMs) noexcept = 0;

        [[nodiscard]] virtual std::wstring GetTransactionLog() const = 0;
    };

}
