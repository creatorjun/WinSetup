// src/abstractions/messaging/IDispatcher.h 
#pragma once

#include <functional>
#include <chrono>

namespace winsetup::abstractions {

    enum class DispatchPriority {
        Low,
        Normal,
        High,
        Immediate
    };

    class IDispatcher {
    public:
        virtual ~IDispatcher() = default;

        virtual void Invoke(std::function<void()> action) = 0;
        virtual void InvokeAsync(std::function<void()> action) = 0;
        virtual void InvokeDelayed(
            std::function<void()> action,
            std::chrono::milliseconds delay
        ) = 0;
        virtual void InvokeWithPriority(
            std::function<void()> action,
            DispatchPriority priority
        ) = 0;

        virtual bool IsOnDispatcherThread() const = 0;
        virtual void ProcessEvents() = 0;
        virtual void Shutdown() = 0;
    };

}
