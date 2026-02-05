// src/abstractions/async/IAwaitable.h 
#pragma once

#include <optional>
#include <coroutine>
#include <exception>

namespace winsetup::abstractions {

    enum class AwaitableState {
        Pending,
        Ready,
        Completed,
        Cancelled,
        Failed
    };

    template<typename T = void>
    class IAwaitable {
    public:
        virtual ~IAwaitable() = default;

        [[nodiscard]] virtual bool IsReady() const noexcept = 0;
        [[nodiscard]] virtual AwaitableState GetState() const noexcept = 0;

        virtual void Wait() = 0;

        template<typename Rep, typename Period>
        [[nodiscard]] bool WaitFor(
            const std::chrono::duration<Rep, Period>& timeout
        ) {
            return WaitForImpl(
                std::chrono::duration_cast<std::chrono::milliseconds>(timeout)
            );
        }

        template<typename Clock, typename Duration>
        [[nodiscard]] bool WaitUntil(
            const std::chrono::time_point<Clock, Duration>& timePoint
        ) {
            const auto now = Clock::now();
            if (timePoint <= now) {
                return IsReady();
            }
            return WaitFor(timePoint - now);
        }

        virtual void Cancel() = 0;

        [[nodiscard]] virtual bool IsCancelled() const noexcept = 0;

        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct Awaiter {
            IAwaitable& awaitable;

            [[nodiscard]] bool await_ready() const noexcept {
                return awaitable.IsReady();
            }

            void await_suspend(std::coroutine_handle<> continuation) {
                awaitable.SetContinuation(continuation);
            }

            T await_resume() {
                if constexpr (std::is_void_v<T>) {
                    awaitable.GetResult();
                }
                else {
                    return awaitable.GetResult();
                }
            }
        };

        [[nodiscard]] Awaiter operator co_await() noexcept {
            return Awaiter{ *this };
        }

    protected:
        virtual void SetContinuation(std::coroutine_handle<> continuation) = 0;

        [[nodiscard]] virtual T GetResult() = 0;

        [[nodiscard]] virtual bool WaitForImpl(
            std::chrono::milliseconds timeout
        ) = 0;
    };

    template<typename T>
    struct IAwaitable<T>::promise_type {
        std::optional<T> result;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;
        AwaitableState state{ AwaitableState::Pending };

        IAwaitable<T> get_return_object() {
            return IAwaitable<T>{ handle_type::from_promise(*this) };
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            state = AwaitableState::Completed;
            if (continuation) {
                continuation.resume();
            }
            return {};
        }

        void return_value(T value) {
            result = std::move(value);
            state = AwaitableState::Ready;
        }

        void unhandled_exception() {
            exception = std::current_exception();
            state = AwaitableState::Failed;
        }
    };

    template<>
    struct IAwaitable<void>::promise_type {
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;
        AwaitableState state{ AwaitableState::Pending };

        IAwaitable<void> get_return_object() {
            return IAwaitable<void>{ handle_type::from_promise(*this) };
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            state = AwaitableState::Completed;
            if (continuation) {
                continuation.resume();
            }
            return {};
        }

        void return_void() {
            state = AwaitableState::Ready;
        }

        void unhandled_exception() {
            exception = std::current_exception();
            state = AwaitableState::Failed;
        }
    };

}
