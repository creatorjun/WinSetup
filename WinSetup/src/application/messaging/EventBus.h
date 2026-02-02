#pragma once

#include <unordered_map>
#include <vector>
#include <mutex>
#include <typeindex>
#include <functional>
#include <atomic>
#include "../../abstractions/messaging/IEventBus.h"

namespace winsetup::application {

    class EventBus : public abstractions::IEventBus {
    public:
        EventBus();
        ~EventBus() override;

        void Unsubscribe(abstractions::SubscriptionToken token) override;
        void Clear() override;
        size_t GetSubscriberCount() const override;

        template<typename TEvent>
        size_t GetSubscriberCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(std::type_index(typeid(TEvent)));
            return it != subscribers_.end() ? it->second.size() : 0;
        }

    protected:
        abstractions::SubscriptionToken SubscribeImpl(
            std::type_index eventType,
            std::function<void(const void*)> handler
        ) override;

        void PublishImpl(
            std::type_index eventType,
            const void* event
        ) override;

    private:
        struct Subscription {
            abstractions::SubscriptionToken token;
            std::function<void(const void*)> handler;
        };

        mutable std::mutex mutex_;
        std::unordered_map<std::type_index, std::vector<Subscription>> subscribers_;
        std::unordered_map<abstractions::SubscriptionToken, std::type_index> tokenToType_;
        std::atomic<abstractions::SubscriptionToken> nextToken_;
    };

}
