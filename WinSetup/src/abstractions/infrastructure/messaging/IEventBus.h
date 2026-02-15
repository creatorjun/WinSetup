// src/abstractions/infrastructure/messaging/IEventBus.h
#pragma once

#include "IEvent.h"
#include <domain/primitives/Expected.h>
#include <functional>
#include <memory>
#include <typeindex>
#include <cstdint>

namespace winsetup::abstractions {

    using EventHandler = std::function<void(const IEvent&)>;
    using SubscriptionToken = uint64_t;

    enum class EventPriority {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3
    };

    struct EventStatistics {
        size_t totalPublished;
        size_t totalSubscriptions;
        size_t activeSubscriptions;
        size_t failedDeliveries;

        EventStatistics()
            : totalPublished(0)
            , totalSubscriptions(0)
            , activeSubscriptions(0)
            , failedDeliveries(0)
        {
        }
    };

    class IEventBus {
    public:
        virtual ~IEventBus() = default;

        [[nodiscard]] virtual domain::Expected<void> Publish(
            std::unique_ptr<IEvent> event,
            EventPriority priority = EventPriority::Normal
        ) = 0;

        template<typename TEvent>
        [[nodiscard]] domain::Expected<void> Publish(
            TEvent event,
            EventPriority priority = EventPriority::Normal
        ) {
            static_assert(std::is_base_of_v<IEvent, TEvent>, "TEvent must derive from IEvent");
            return Publish(std::make_unique<TEvent>(std::move(event)), priority);
        }

        [[nodiscard]] virtual domain::Expected<SubscriptionToken> Subscribe(
            std::type_index eventType,
            EventHandler handler
        ) = 0;

        template<typename TEvent>
        [[nodiscard]] domain::Expected<SubscriptionToken> Subscribe(
            std::function<void(const TEvent&)> handler
        ) {
            static_assert(std::is_base_of_v<IEvent, TEvent>, "TEvent must derive from IEvent");

            auto wrapper = [handler](const IEvent& event) {
                const TEvent& typedEvent = static_cast<const TEvent&>(event);
                handler(typedEvent);
                };

            return Subscribe(std::type_index(typeid(TEvent)), wrapper);
        }

        [[nodiscard]] virtual domain::Expected<void> Unsubscribe(
            SubscriptionToken token
        ) = 0;

        virtual void UnsubscribeAll() = 0;

        [[nodiscard]] virtual size_t GetSubscriptionCount() const noexcept = 0;
        [[nodiscard]] virtual size_t GetSubscriptionCount(std::type_index eventType) const noexcept = 0;

        [[nodiscard]] virtual EventStatistics GetStatistics() const noexcept = 0;
        virtual void ResetStatistics() noexcept = 0;

        virtual void SetMaxQueueSize(size_t maxSize) noexcept = 0;
        [[nodiscard]] virtual size_t GetQueuedEventCount() const noexcept = 0;
    };

}
