// src/application/messaging/MessageQueue.h 
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "../../abstractions/messaging/IMessageQueue.h"

namespace winsetup::application {

    template<typename T>
    class MessageQueue : public abstractions::IMessageQueue<T> {
    public:
        MessageQueue() = default;
        ~MessageQueue() override = default;

        void Enqueue(T message) override {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(message));
            cv_.notify_one();
        }

        std::optional<T> Dequeue() override {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !queue_.empty(); });

            if (queue_.empty()) {
                return std::nullopt;
            }

            T message = std::move(queue_.front());
            queue_.pop();
            return message;
        }

        std::optional<T> TryDequeue() override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return std::nullopt;
            }

            T message = std::move(queue_.front());
            queue_.pop();
            return message;
        }

        bool IsEmpty() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        size_t Size() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        void Clear() override {
            std::lock_guard<std::mutex> lock(mutex_);
            while (!queue_.empty()) {
                queue_.pop();
            }
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<T> queue_;
    };

}
