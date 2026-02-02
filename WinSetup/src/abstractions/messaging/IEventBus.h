#pragma once

#include <functional>
#include <memory>
#include <typeindex>

namespace winsetup::abstractions {

    using SubscriptionToken = size_t;

    class IEventBus {
    public:
        virtual ~IEventBus() = default;

        template<typename TEvent>
        SubscriptionToken Subscribe(std::function<void(const TEvent&)> handler) {
            return SubscribeImpl(
                std::type_index(typeid(TEvent)),
                [handler](const void* event) {
                    handler(*static_cast<const TEvent*>(event));
                }
            );
        }

        template<typename TEvent>
        void Publish(const TEvent& event) {
            PublishImpl(std::type_index(typeid(TEvent)), &event);
        }

        virtual void Unsubscribe(SubscriptionToken token) = 0;
        virtual void Clear() = 0;
        virtual size_t GetSubscriberCount() const = 0;

    protected:
        virtual SubscriptionToken SubscribeImpl(
            std::type_index eventType,
            std::function<void(const void*)> handler
        ) = 0;

        virtual void PublishImpl(
            std::type_index eventType,
            const void* event
        ) = 0;
    };

}
