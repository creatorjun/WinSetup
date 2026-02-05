#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "../../abstractions/async/IAwaitable.h"

namespace winsetup::application {

    template<typename T = void>
    class Awaitable : public abstractions::IAwaitable<T> {
    public:
        Awaitable() = default;

        explicit Awaitable(std::coroutine_handle<typename abstractions::IAwaitable<T>::promise_type> handle)
            : handle_(handle) {
        }

        ~Awaitable() override {
            if (handle_ && !handle_.done()) {
                handle_.destroy();
            }
        }

        Awaitable(const Awaitable&) = delete;
        Awaitable& operator=(const Awaitable&) = delete;

        Awaitable(Awaitable&& other) noexcept
            : handle_(std::exchange(other.handle_, nullptr))
            , state_(other.state_.load())
            , continuation_(std::exchange(other.continuation_, nullptr)) {
        }

        Awaitable& operator=(Awaitable&& other) noexcept {
            if (this != &other) {
                if (handle_ && !handle_.done()) {
                    handle_.destroy();
                }
                handle_ = std::exchange(other.handle_, nullptr);
                state_.store(other.state_.load());
                continuation_ = std::exchange(other.continuation_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] bool IsReady() const noexcept override {
            const auto state = state_.load(std::memory_order_acquire);
            return state == abstractions::AwaitableState::Ready ||
                state == abstractions::AwaitableState::Completed;
        }

        [[nodiscard]] abstractions::AwaitableState GetState() const noexcept override {
            return state_.load(std::memory_order_acquire);
        }

        void Wait() override {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return IsReady(); });
        }

        void Cancel() override {
            state_.store(abstractions::AwaitableState::Cancelled, std::memory_order_release);
            cv_.notify_all();
        }

        [[nodiscard]] bool IsCancelled() const noexcept override {
            return state_.load(std::memory_order_acquire) == abstractions::AwaitableState::Cancelled;
        }

        void SetResult(T value) requires (!std::is_void_v<T>) {
            std::lock_guard<std::mutex> lock(mutex_);
            result_ = std::move(value);
            state_.store(abstractions::AwaitableState::Ready, std::memory_order_release);
            cv_.notify_all();

            if (continuation_) {
                continuation_.resume();
            }
        }

        void SetResult() requires std::is_void_v<T> {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.store(abstractions::AwaitableState::Ready, std::memory_order_release);
            cv_.notify_all();

            if (continuation_) {
                continuation_.resume();
            }
        }

        void SetException(std::exception_ptr exception) {
            std::lock_guard<std::mutex> lock(mutex_);
            exception_ = exception;
            state_.store(abstractions::AwaitableState::Failed, std::memory_order_release);
            cv_.notify_all();

            if (continuation_) {
                continuation_.resume();
            }
        }

    protected:
        void SetContinuation(std::coroutine_handle<> continuation) override {
            std::lock_guard<std::mutex> lock(mutex_);
            continuation_ = continuation;
        }

        [[nodiscard]] T GetResult() override {
            std::lock_guard<std::mutex> lock(mutex_);

            if (exception_) {
                std::rethrow_exception(exception_);
            }

            if (state_.load(std::memory_order_acquire) == abstractions::AwaitableState::Cancelled) {
                throw std::runtime_error("Operation was cancelled");
            }

            if constexpr (!std::is_void_v<T>) {
                if (!result_.has_value()) {
                    throw std::runtime_error("Result not available");
                }
                return std::move(result_.value());
            }
        }

        [[nodiscard]] bool WaitForImpl(std::chrono::milliseconds timeout) override {
            std::unique_lock<std::mutex> lock(mutex_);
            return cv_.wait_for(lock, timeout, [this] { return IsReady(); });
        }

    private:
        std::coroutine_handle<typename abstractions::IAwaitable<T>::promise_type> handle_;
        std::atomic<abstractions::AwaitableState> state_{ abstractions::AwaitableState::Pending };
        std::coroutine_handle<> continuation_;
        std::optional<T> result_;
        std::exception_ptr exception_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
    };

    template<typename T>
    [[nodiscard]] inline Awaitable<T> MakeReadyAwaitable(T value) {
        Awaitable<T> awaitable;
        awaitable.SetResult(std::move(value));
        return awaitable;
    }

    [[nodiscard]] inline Awaitable<void> MakeReadyAwaitable() {
        Awaitable<void> awaitable;
        awaitable.SetResult();
        return awaitable;
    }

    template<typename T>
    [[nodiscard]] inline Awaitable<T> MakeFailedAwaitable(std::exception_ptr exception) {
        Awaitable<T> awaitable;
        awaitable.SetException(exception);
        return awaitable;
    }

}
