// src/application/messaging/Dispatcher.h 
#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <abstractions/messaging/IDispatcher.h>
#include "MessageQueue.h"

namespace winsetup::application {

    class Dispatcher : public abstractions::IDispatcher {
    public:
        Dispatcher();
        ~Dispatcher() override;

        void Invoke(std::function<void()> action) override;
        void InvokeAsync(std::function<void()> action) override;
        void InvokeDelayed(
            std::function<void()> action,
            std::chrono::milliseconds delay
        ) override;
        void InvokeWithPriority(
            std::function<void()> action,
            abstractions::DispatchPriority priority
        ) override;

        bool IsOnDispatcherThread() const override;
        void ProcessEvents() override;
        void Shutdown() override;

    private:
        struct Message {
            std::function<void()> action;
            abstractions::DispatchPriority priority;
            std::chrono::steady_clock::time_point scheduledTime;

            Message()
                : priority(abstractions::DispatchPriority::Normal)
                , scheduledTime(std::chrono::steady_clock::now()) {
            }

            Message(
                std::function<void()> act,
                abstractions::DispatchPriority prio = abstractions::DispatchPriority::Normal
            ) : action(std::move(act))
                , priority(prio)
                , scheduledTime(std::chrono::steady_clock::now()) {
            }

            bool operator<(const Message& other) const {
                if (priority != other.priority) {
                    return static_cast<int>(priority) < static_cast<int>(other.priority);
                }
                return scheduledTime > other.scheduledTime;
            }
        };

        void ProcessMessage(const Message& message);

        std::thread::id dispatcherThreadId_;
        MessageQueue<Message> messageQueue_;
        std::atomic<bool> shutdown_;
    };

}
