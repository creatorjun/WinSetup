// src/abstractions/infrastructure/messaging/IDispatcher.h
#pragma once

#include "IEvent.h"
#include <domain/primitives/Expected.h>
#include <functional>
#include <memory>
#include <cstdint>

namespace winsetup::abstractions {

    enum class DispatchMode {
        Synchronous,
        Asynchronous,
        Deferred
    };

    enum class DispatchPriority {
        Immediate = 0,
        High = 1,
        Normal = 2,
        Low = 3,
        Background = 4
    };

    struct DispatchOptions {
        DispatchMode mode;
        DispatchPriority priority;
        uint32_t timeoutMs;
        bool guaranteedDelivery;

        DispatchOptions()
            : mode(DispatchMode::Asynchronous)
            , priority(DispatchPriority::Normal)
            , timeoutMs(5000)
            , guaranteedDelivery(false)
        {
        }
    };

    using DispatchCallback = std::function<void(bool success, const std::wstring& error)>;

    class IDispatcher {
    public:
        virtual ~IDispatcher() = default;

        [[nodiscard]] virtual domain::Expected<void> Dispatch(
            std::unique_ptr<IEvent> event,
            const DispatchOptions& options = DispatchOptions{}
        ) = 0;

        template<typename TEvent>
        [[nodiscard]] domain::Expected<void> Dispatch(
            TEvent event,
            const DispatchOptions& options = DispatchOptions{}
        ) {
            static_assert(std::is_base_of_v<IEvent, TEvent>, "TEvent must derive from IEvent");
            return Dispatch(std::make_unique<TEvent>(std::move(event)), options);
        }

        [[nodiscard]] virtual domain::Expected<void> DispatchAsync(
            std::unique_ptr<IEvent> event,
            DispatchCallback callback = nullptr,
            const DispatchOptions& options = DispatchOptions{}
        ) = 0;

        [[nodiscard]] virtual domain::Expected<void> DispatchBatch(
            std::vector<std::unique_ptr<IEvent>> events,
            const DispatchOptions& options = DispatchOptions{}
        ) = 0;

        virtual void CancelPending() = 0;
        [[nodiscard]] virtual size_t GetPendingCount() const noexcept = 0;
        [[nodiscard]] virtual size_t GetDispatchedCount() const noexcept = 0;

        virtual void SetMaxConcurrency(size_t maxConcurrency) noexcept = 0;
        [[nodiscard]] virtual size_t GetMaxConcurrency() const noexcept = 0;

        virtual void Pause() noexcept = 0;
        virtual void Resume() noexcept = 0;
        [[nodiscard]] virtual bool IsPaused() const noexcept = 0;
    };

}
