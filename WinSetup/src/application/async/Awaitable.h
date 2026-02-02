#pragma once

#include <coroutine>
#include <optional>
#include <functional>
#include "../../domain/primitives/Expected.h"
#include "../../domain/primitives/Result.h"
#include "../../domain/primitives/Error.h"
#include "Promise.h"

namespace winsetup::application {

    template<typename T>
    class Awaitable {
    public:
        explicit Awaitable(Future<T> future)
            : future_(std::move(future)) {
        }

        bool await_ready() const noexcept {
            return future_.IsReady();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            future_.Then([handle](const winsetup::domain::Expected<T>&) mutable {
                handle.resume();
                });
        }

        winsetup::domain::Expected<T> await_resume() {
            return future_.Get();
        }

    private:
        Future<T> future_;
    };

    template<>
    class Awaitable<void> {
    public:
        explicit Awaitable(Future<void> future)
            : future_(std::move(future)) {
        }

        bool await_ready() const noexcept {
            return future_.IsReady();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            future_.Then([handle](const winsetup::domain::Result<>&) mutable {
                handle.resume();
                });
        }

        winsetup::domain::Result<> await_resume() {
            return future_.Get();
        }

    private:
        Future<void> future_;
    };

    template<typename T>
    Awaitable<T> MakeAwaitable(Future<T> future) {
        return Awaitable<T>(std::move(future));
    }

    template<typename T>
    class DelayedAwaitable {
    public:
        explicit DelayedAwaitable(std::chrono::milliseconds delay)
            : delay_(delay) {
        }

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            std::thread([handle, delay = delay_]() {
                std::this_thread::sleep_for(delay);
                handle.resume();
                }).detach();
        }

        void await_resume() const noexcept {
        }

    private:
        std::chrono::milliseconds delay_;
    };

    inline DelayedAwaitable<void> Delay(std::chrono::milliseconds duration) {
        return DelayedAwaitable<void>(duration);
    }

    template<typename T>
    class YieldAwaitable {
    public:
        YieldAwaitable() = default;

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) const noexcept {
            handle.resume();
        }

        void await_resume() const noexcept {
        }
    };

    inline YieldAwaitable<void> Yield() {
        return YieldAwaitable<void>();
    }

    template<typename T>
    class ValueAwaitable {
    public:
        explicit ValueAwaitable(T value)
            : value_(std::move(value)) {
        }

        bool await_ready() const noexcept {
            return true;
        }

        void await_suspend(std::coroutine_handle<>) const noexcept {
        }

        T await_resume() {
            return std::move(value_);
        }

    private:
        T value_;
    };

    template<typename T>
    ValueAwaitable<T> MakeReadyAwaitable(T value) {
        return ValueAwaitable<T>(std::move(value));
    }

    template<typename T, typename Executor>
    class AwaitableOnExecutor {
    public:
        AwaitableOnExecutor(Awaitable<T> awaitable, Executor executor)
            : awaitable_(std::move(awaitable))
            , executor_(std::move(executor)) {
        }

        bool await_ready() const noexcept {
            return awaitable_.await_ready();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            executor_([this, handle]() mutable {
                awaitable_.await_suspend(handle);
                });
        }

        auto await_resume() {
            return awaitable_.await_resume();
        }

    private:
        Awaitable<T> awaitable_;
        Executor executor_;
    };

    template<typename T, typename Executor>
    AwaitableOnExecutor<T, Executor> AwaitOn(Awaitable<T> awaitable, Executor executor) {
        return AwaitableOnExecutor<T, Executor>(std::move(awaitable), std::move(executor));
    }

}
