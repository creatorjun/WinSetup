#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <optional>
#include <functional>
#include <condition_variable>
#include <domain/primitives/Error.h>
#include <domain/primitives/Expected.h>

namespace winsetup::application {

    template<typename T>
    class Promise;

    template<typename T>
    class Future {
    public:
        Future() = default;

        [[nodiscard]] bool IsReady() const noexcept {
            if (!state_) [[unlikely]] return false;
            return state_->ready.load(std::memory_order_acquire);
        }

        void Wait() const {
            if (!state_) [[unlikely]] return;
            std::unique_lock<std::mutex> lock(state_->mutex);
            state_->cv.wait(lock, [this]() {
                return state_->ready.load(std::memory_order_acquire);
                });
        }

        template<typename Rep, typename Period>
        [[nodiscard]] bool WaitFor(const std::chrono::duration<Rep, Period>& duration) const {
            if (!state_) [[unlikely]] return false;
            std::unique_lock<std::mutex> lock(state_->mutex);
            return state_->cv.wait_for(lock, duration, [this]() {
                return state_->ready.load(std::memory_order_acquire);
                });
        }

        [[nodiscard]] winsetup::domain::Expected<T> Get() const {
            if (!state_) [[unlikely]] {
                return winsetup::domain::Expected<T>::Failure(
                    winsetup::domain::Error("Invalid future")
                );
            }
            Wait();
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (!state_->result.has_value()) [[unlikely]] {
                return winsetup::domain::Expected<T>::Failure(
                    winsetup::domain::Error("Result not set")
                );
            }
            return state_->result.value();
        }

        void Then(std::function<void(const winsetup::domain::Expected<T>&)> callback) {
            if (!state_) [[unlikely]] return;

            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->ready.load(std::memory_order_acquire)) [[likely]] {
                if (callback && state_->result.has_value()) {
                    lock.~lock_guard();
                    callback(state_->result.value());
                }
            }
            else {
                state_->callbacks.push_back(std::move(callback));
            }
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return state_ != nullptr;
        }

    private:
        struct SharedState {
            mutable std::mutex mutex;
            std::condition_variable cv;
            std::atomic<bool> ready{ false };
            std::optional<winsetup::domain::Expected<T>> result;
            std::vector<std::function<void(const winsetup::domain::Expected<T>&)>> callbacks;
        };

        explicit Future(std::shared_ptr<SharedState> state)
            : state_(std::move(state)) {
        }

        std::shared_ptr<SharedState> state_;

        friend class Promise<T>;
    };

    template<typename T>
    class Promise {
    public:
        Promise() : state_(std::make_shared<typename Future<T>::SharedState>()) {
        }

        [[nodiscard]] Future<T> GetFuture() const {
            return Future<T>(state_);
        }

        void SetValue(T value) {
            std::vector<std::function<void(const winsetup::domain::Expected<T>&)>> callbacks;
            winsetup::domain::Expected<T> result =
                winsetup::domain::Expected<T>::Success(std::move(value));
            {
                std::lock_guard<std::mutex> lock(state_->mutex);
                if (state_->ready.load(std::memory_order_acquire)) [[unlikely]] {
                    return;
                }
                state_->result = result;
                state_->ready.store(true, std::memory_order_release);
                callbacks = std::move(state_->callbacks);
            }
            state_->cv.notify_all();
            for (auto& callback : callbacks) {
                if (callback) [[likely]] {
                    callback(result);
                }
            }
        }

        void SetError(winsetup::domain::Error error) {
            std::vector<std::function<void(const winsetup::domain::Expected<T>&)>> callbacks;
            winsetup::domain::Expected<T> result =
                winsetup::domain::Expected<T>::Failure(std::move(error));
            {
                std::lock_guard<std::mutex> lock(state_->mutex);
                if (state_->ready.load(std::memory_order_acquire)) [[unlikely]] {
                    return;
                }
                state_->result = result;
                state_->ready.store(true, std::memory_order_release);
                callbacks = std::move(state_->callbacks);
            }
            state_->cv.notify_all();
            for (auto& callback : callbacks) {
                if (callback) [[likely]] {
                    callback(result);
                }
            }
        }

        void SetException(std::exception_ptr exception) {
            try {
                if (exception) [[likely]] {
                    std::rethrow_exception(exception);
                }
            }
            catch (const std::exception& e) {
                SetError(winsetup::domain::Error(e.what()));
            }
            catch (...) {
                SetError(winsetup::domain::Error("Unknown exception"));
            }
        }

        [[nodiscard]] bool IsSet() const noexcept {
            return state_->ready.load(std::memory_order_acquire);
        }

    private:
        std::shared_ptr<typename Future<T>::SharedState> state_;
    };

    template<>
    class Future<void> {
    public:
        Future() = default;

        [[nodiscard]] bool IsReady() const noexcept {
            if (!state_) [[unlikely]] return false;
            return state_->ready.load(std::memory_order_acquire);
        }

        void Wait() const {
            if (!state_) [[unlikely]] return;
            std::unique_lock<std::mutex> lock(state_->mutex);
            state_->cv.wait(lock, [this]() {
                return state_->ready.load(std::memory_order_acquire);
                });
        }

        template<typename Rep, typename Period>
        [[nodiscard]] bool WaitFor(const std::chrono::duration<Rep, Period>& duration) const {
            if (!state_) [[unlikely]] return false;
            std::unique_lock<std::mutex> lock(state_->mutex);
            return state_->cv.wait_for(lock, duration, [this]() {
                return state_->ready.load(std::memory_order_acquire);
                });
        }

        [[nodiscard]] winsetup::domain::Result<> Get() const {
            if (!state_) [[unlikely]] {
                return winsetup::domain::Result<>::Failure(
                    winsetup::domain::Error("Invalid future")
                );
            }
            Wait();
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (!state_->result.has_value()) [[unlikely]] {
                return winsetup::domain::Result<>::Failure(
                    winsetup::domain::Error("Result not set")
                );
            }
            return state_->result.value();
        }

        void Then(std::function<void(const winsetup::domain::Result<>&)> callback) {
            if (!state_) [[unlikely]] return;

            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->ready.load(std::memory_order_acquire)) [[likely]] {
                if (callback && state_->result.has_value()) {
                    lock.~lock_guard();
                    callback(state_->result.value());
                }
            }
            else {
                state_->callbacks.push_back(std::move(callback));
            }
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return state_ != nullptr;
        }

    private:
        struct SharedState {
            mutable std::mutex mutex;
            std::condition_variable cv;
            std::atomic<bool> ready{ false };
            std::optional<winsetup::domain::Result<>> result;
            std::vector<std::function<void(const winsetup::domain::Result<>&)>> callbacks;
        };

        explicit Future(std::shared_ptr<SharedState> state)
            : state_(std::move(state)) {
        }

        std::shared_ptr<SharedState> state_;

        friend class Promise<void>;
    };

    template<>
    class Promise<void> {
    public:
        Promise() : state_(std::make_shared<Future<void>::SharedState>()) {
        }

        [[nodiscard]] Future<void> GetFuture() const {
            return Future<void>(state_);
        }

        void SetValue() {
            std::vector<std::function<void(const winsetup::domain::Result<>&)>> callbacks;
            winsetup::domain::Result<> result =
                winsetup::domain::Result<>::Success();
            {
                std::lock_guard<std::mutex> lock(state_->mutex);
                if (state_->ready.load(std::memory_order_acquire)) [[unlikely]] {
                    return;
                }
                state_->result = result;
                state_->ready.store(true, std::memory_order_release);
                callbacks = std::move(state_->callbacks);
            }
            state_->cv.notify_all();
            for (auto& callback : callbacks) {
                if (callback) [[likely]] {
                    callback(result);
                }
            }
        }

        void SetError(winsetup::domain::Error error) {
            std::vector<std::function<void(const winsetup::domain::Result<>&)>> callbacks;
            winsetup::domain::Result<> result =
                winsetup::domain::Result<>::Failure(std::move(error));
            {
                std::lock_guard<std::mutex> lock(state_->mutex);
                if (state_->ready.load(std::memory_order_acquire)) [[unlikely]] {
                    return;
                }
                state_->result = result;
                state_->ready.store(true, std::memory_order_release);
                callbacks = std::move(state_->callbacks);
            }
            state_->cv.notify_all();
            for (auto& callback : callbacks) {
                if (callback) [[likely]] {
                    callback(result);
                }
            }
        }

        void SetException(std::exception_ptr exception) {
            try {
                if (exception) [[likely]] {
                    std::rethrow_exception(exception);
                }
            }
            catch (const std::exception& e) {
                SetError(winsetup::domain::Error(e.what()));
            }
            catch (...) {
                SetError(winsetup::domain::Error("Unknown exception"));
            }
        }

        [[nodiscard]] bool IsSet() const noexcept {
            return state_->ready.load(std::memory_order_acquire);
        }

    private:
        std::shared_ptr<Future<void>::SharedState> state_;
    };

}
