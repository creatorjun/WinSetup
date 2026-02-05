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

        MessageQueue(const MessageQueue&) = delete;
        MessageQueue& operator=(const MessageQueue&) = delete;
        MessageQueue(MessageQueue&&) = delete;
        MessageQueue& operator=(MessageQueue&&) = delete;

        void Enqueue(T message) override {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(std::move(message));
            }
            cv_.notify_one();
        }

        [[nodiscard]] std::optional<T> Dequeue() override {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !queue_.empty(); });
            return DequeueImpl();
        }

        [[nodiscard]] std::optional<T> TryDequeue() override {
            std::lock_guard<std::mutex> lock(mutex_);
            return DequeueImpl();
        }

        [[nodiscard]] bool IsEmpty() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        [[nodiscard]] size_t Size() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        void Clear() override {
            std::lock_guard<std::mutex> lock(mutex_);
            std::queue<T> empty;
            std::swap(queue_, empty);
        }

    private:
        [[nodiscard]] std::optional<T> DequeueImpl() {
            if (queue_.empty()) [[unlikely]] {
                return std::nullopt;
            }
            T message = std::move(queue_.front());
            queue_.pop();
            return message;
        }

        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<T> queue_;
    };

}
