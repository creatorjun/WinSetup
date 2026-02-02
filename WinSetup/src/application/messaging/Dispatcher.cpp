// src/application/messaging/Dispatcher.cpp 
#include "Dispatcher.h"
#include <chrono>

namespace winsetup::application {

    Dispatcher::Dispatcher()
        : dispatcherThreadId_(std::this_thread::get_id())
        , shutdown_(false) {
    }

    Dispatcher::~Dispatcher() {
        Shutdown();
    }

    void Dispatcher::Invoke(std::function<void()> action) {
        if (IsOnDispatcherThread()) {
            if (action) {
                action();
            }
        }
        else {
            InvokeAsync(std::move(action));
        }
    }

    void Dispatcher::InvokeAsync(std::function<void()> action) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        Message message(std::move(action), abstractions::DispatchPriority::Normal);
        messageQueue_.Enqueue(std::move(message));
    }

    void Dispatcher::InvokeDelayed(
        std::function<void()> action,
        std::chrono::milliseconds delay
    ) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        auto scheduledTime = std::chrono::steady_clock::now() + delay;

        std::thread([this, action = std::move(action), scheduledTime]() mutable {
            std::this_thread::sleep_until(scheduledTime);
            if (!shutdown_.load(std::memory_order_acquire)) {
                Message message(std::move(action), abstractions::DispatchPriority::Normal);
                message.scheduledTime = scheduledTime;
                messageQueue_.Enqueue(std::move(message));
            }
            }).detach();
    }

    void Dispatcher::InvokeWithPriority(
        std::function<void()> action,
        abstractions::DispatchPriority priority
    ) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        Message message(std::move(action), priority);
        messageQueue_.Enqueue(std::move(message));
    }

    bool Dispatcher::IsOnDispatcherThread() const {
        return std::this_thread::get_id() == dispatcherThreadId_;
    }

    void Dispatcher::ProcessEvents() {
        while (!shutdown_.load(std::memory_order_acquire)) {
            auto message = messageQueue_.TryDequeue();
            if (message.has_value()) {
                ProcessMessage(message.value());
            }
            else {
                break;
            }
        }
    }

    void Dispatcher::Shutdown() {
        bool expected = false;
        if (shutdown_.compare_exchange_strong(expected, true)) {
            messageQueue_.Clear();
        }
    }

    void Dispatcher::ProcessMessage(const Message& message) {
        if (message.action) {
            try {
                message.action();
            }
            catch (...) {
            }
        }
    }

}
