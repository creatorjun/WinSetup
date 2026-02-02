#include "EventBus.h"
#include <algorithm>

namespace winsetup::application {

    EventBus::EventBus() : nextToken_(1) {
    }

    EventBus::~EventBus() {
        Clear();
    }

    abstractions::SubscriptionToken EventBus::SubscribeImpl(
        std::type_index eventType,
        std::function<void(const void*)> handler
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto token = nextToken_.fetch_add(1, std::memory_order_relaxed);

        Subscription sub;
        sub.token = token;
        sub.handler = std::move(handler);

        subscribers_[eventType].push_back(std::move(sub));
        tokenToType_.insert({ token, eventType });

        return token;
    }

    void EventBus::PublishImpl(
        std::type_index eventType,
        const void* event
    ) {
        std::vector<std::function<void(const void*)>> handlers;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(eventType);
            if (it != subscribers_.end()) {
                handlers.reserve(it->second.size());
                for (const auto& sub : it->second) {
                    handlers.push_back(sub.handler);
                }
            }
        }

        for (const auto& handler : handlers) {
            if (handler) {
                try {
                    handler(event);
                }
                catch (...) {
                }
            }
        }
    }

    void EventBus::Unsubscribe(abstractions::SubscriptionToken token) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto typeIt = tokenToType_.find(token);
        if (typeIt == tokenToType_.end()) {
            return;
        }

        auto eventType = typeIt->second;
        auto subsIt = subscribers_.find(eventType);
        if (subsIt != subscribers_.end()) {
            auto& subs = subsIt->second;
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [token](const Subscription& sub) {
                        return sub.token == token;
                    }),
                subs.end()
            );

            if (subs.empty()) {
                subscribers_.erase(subsIt);
            }
        }

        tokenToType_.erase(typeIt);
    }

    void EventBus::Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_.clear();
        tokenToType_.clear();
    }

    size_t EventBus::GetSubscriberCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& pair : subscribers_) {
            count += pair.second.size();
        }
        return count;
    }

}
