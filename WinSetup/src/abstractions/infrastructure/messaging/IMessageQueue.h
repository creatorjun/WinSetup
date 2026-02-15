// src/abstractions/infrastructure/messaging/IMessageQueue.h
#pragma once

#include "IEvent.h"
#include <domain/primitives/Expected.h>
#include <memory>
#include <chrono>
#include <optional>

namespace winsetup::abstractions {

    enum class QueuePolicy {
        FIFO,
        LIFO,
        Priority
    };

    class IMessageQueue {
    public:
        virtual ~IMessageQueue() = default;

        [[nodiscard]] virtual domain::Expected<void> Enqueue(
            std::unique_ptr<IEvent> event
        ) = 0;

        [[nodiscard]] virtual domain::Expected<std::unique_ptr<IEvent>> Dequeue(
            std::chrono::milliseconds timeout = std::chrono::milliseconds::zero()
        ) = 0;

        [[nodiscard]] virtual std::optional<std::unique_ptr<IEvent>> TryDequeue() = 0;

        [[nodiscard]] virtual size_t GetSize() const noexcept = 0;
        [[nodiscard]] virtual size_t GetCapacity() const noexcept = 0;
        [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;
        [[nodiscard]] virtual bool IsFull() const noexcept = 0;

        virtual void Clear() = 0;
        virtual void SetCapacity(size_t capacity) = 0;
        virtual void SetPolicy(QueuePolicy policy) = 0;

        [[nodiscard]] virtual QueuePolicy GetPolicy() const noexcept = 0;
    };

}
