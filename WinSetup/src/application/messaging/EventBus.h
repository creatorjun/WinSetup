#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <atomic>
#include <abstractions/messaging/IEventBus.h>

namespace winsetup::application {

    class EventBus : public abstractions::IEventBus {
    public:
        EventBus();
        ~EventBus() override;

        void Unsubscribe(abstractions::SubscriptionToken token) override;
        void Clear() override;

        [[nodiscard]] size_t GetSubscriberCount() const override;

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

        using HandlerList = std::shared_ptr<std::vector<Subscription>>;

        mutable std::mutex mutex_;
        std::unordered_map<std::type_index, HandlerList> subscribers_;
        std::unordered_map<abstractions::SubscriptionToken, std::type_index> tokenToType_;
        std::atomic<abstractions::SubscriptionToken> nextToken_;
    };

}
