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

        auto it = subscribers_.find(eventType);
        if (it == subscribers_.end()) {
            auto newList = std::make_shared<std::vector<Subscription>>();
            newList->push_back(std::move(sub));
            subscribers_[eventType] = newList;
        }
        else {
            auto newList = std::make_shared<std::vector<Subscription>>(*it->second);
            newList->push_back(std::move(sub));
            it->second = newList;
        }

        tokenToType_.insert({ token, eventType });

        return token;
    }

    void EventBus::PublishImpl(
        std::type_index eventType,
        const void* event
    ) {
        HandlerList handlers;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(eventType);
            if (it != subscribers_.end()) [[likely]] {
                handlers = it->second;
            }
        }

        if (handlers) [[likely]] {
            for (const auto& sub : *handlers) {
                if (sub.handler) [[likely]] {
                    try {
                        sub.handler(event);
                    }
                    catch (...) {
                    }
                }
            }
        }
    }

    void EventBus::Unsubscribe(abstractions::SubscriptionToken token) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto typeIt = tokenToType_.find(token);
        if (typeIt == tokenToType_.end()) [[unlikely]] {
            return;
        }

        auto eventType = typeIt->second;
        auto subsIt = subscribers_.find(eventType);
        if (subsIt != subscribers_.end()) [[likely]] {
            auto newList = std::make_shared<std::vector<Subscription>>();
            newList->reserve(subsIt->second->size());

            for (const auto& sub : *subsIt->second) {
                if (sub.token != token) {
                    newList->push_back(sub);
                }
            }

            if (newList->empty()) {
                subscribers_.erase(subsIt);
            }
            else {
                subsIt->second = newList;
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
            count += pair.second->size();
        }
        return count;
    }

}
