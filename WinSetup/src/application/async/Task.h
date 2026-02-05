#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <optional>
#include "../../domain/primitives/Expected.h"
#include "../../domain/primitives/Result.h"
#include "../../domain/primitives/Error.h"

namespace winsetup::application {

    template<typename T = void>
    class [[nodiscard]] Task;

    namespace detail {

        template<typename T>
        class TaskPromise {
        public:
            using value_type = T;
            using handle_type = std::coroutine_handle<TaskPromise<T>>;

            Task<T> get_return_object() noexcept;

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_value(T value) noexcept {
                result_ = winsetup::domain::Expected<T>::Success(std::move(value));
            }

            void unhandled_exception() noexcept {
                try {
                    std::rethrow_exception(std::current_exception());
                }
                catch (const std::exception& e) {
                    result_ = winsetup::domain::Expected<T>::Failure(
                        winsetup::domain::Error(e.what())
                    );
                }
                catch (...) {
                    result_ = winsetup::domain::Expected<T>::Failure(
                        winsetup::domain::Error("Unknown exception")
                    );
                }
            }

            [[nodiscard]] winsetup::domain::Expected<T> result() noexcept {
                return result_;
            }

            [[nodiscard]] const winsetup::domain::Expected<T>& result() const noexcept {
                return result_;
            }

        private:
            winsetup::domain::Expected<T> result_{
                winsetup::domain::Expected<T>::Failure(
                    winsetup::domain::Error("Task not completed")
                )
            };
        };

        template<>
        class TaskPromise<void> {
        public:
            using value_type = void;
            using handle_type = std::coroutine_handle<TaskPromise<void>>;

            Task<void> get_return_object() noexcept;

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                return {};
            }

            void return_void() noexcept {
                result_ = winsetup::domain::Result<>::Success();
            }

            void unhandled_exception() noexcept {
                try {
                    std::rethrow_exception(std::current_exception());
                }
                catch (const std::exception& e) {
                    result_ = winsetup::domain::Result<>::Failure(
                        winsetup::domain::Error(e.what())
                    );
                }
                catch (...) {
                    result_ = winsetup::domain::Result<>::Failure(
                        winsetup::domain::Error("Unknown exception")
                    );
                }
            }

            [[nodiscard]] winsetup::domain::Result<> result() noexcept {
                return result_;
            }

            [[nodiscard]] const winsetup::domain::Result<>& result() const noexcept {
                return result_;
            }

        private:
            winsetup::domain::Result<> result_{
                winsetup::domain::Result<>::Failure(
                    winsetup::domain::Error("Task not completed")
                )
            };
        };

    }

    template<typename T>
    class [[nodiscard]] Task {
    public:
        using promise_type = detail::TaskPromise<T>;
        using handle_type = std::coroutine_handle<promise_type>;
        using value_type = T;

        explicit Task(handle_type handle) noexcept
            : handle_(handle) {
        }

        Task(Task&& other) noexcept
            : handle_(std::exchange(other.handle_, nullptr)) {
        }

        Task& operator=(Task&& other) noexcept {
            if (this != &other) [[likely]] {
                if (handle_) {
                    handle_.destroy();
                }
                handle_ = std::exchange(other.handle_, nullptr);
            }
            return *this;
        }

        ~Task() {
            if (handle_) {
                handle_.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        [[nodiscard]] bool Resume() {
            if (!handle_ || handle_.done()) [[unlikely]] {
                return false;
            }
            handle_.resume();
            return !handle_.done();
        }

        [[nodiscard]] bool IsDone() const noexcept {
            return !handle_ || handle_.done();
        }

        [[nodiscard]] winsetup::domain::Expected<T> GetResult() const noexcept {
            if (!handle_) [[unlikely]] {
                return winsetup::domain::Expected<T>::Failure(
                    winsetup::domain::Error("Invalid task handle")
                );
            }
            if (!handle_.done()) [[unlikely]] {
                return winsetup::domain::Expected<T>::Failure(
                    winsetup::domain::Error("Task not completed")
                );
            }
            return handle_.promise().result();
        }

        [[nodiscard]] T GetResultUnchecked() const noexcept {
            return handle_.promise().result().Value();
        }

        void RunToCompletion() {
            while (Resume()) {
            }
        }

        auto operator co_await() const noexcept {
            struct Awaiter {
                handle_type handle;

                [[nodiscard]] bool await_ready() const noexcept {
                    return !handle || handle.done();
                }

                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<> continuation
                ) const noexcept {
                    return handle;
                }

                [[nodiscard]] winsetup::domain::Expected<T> await_resume() const noexcept {
                    if (!handle) [[unlikely]] {
                        return winsetup::domain::Expected<T>::Failure(
                            winsetup::domain::Error("Invalid task handle")
                        );
                    }
                    return handle.promise().result();
                }
            };
            return Awaiter{ handle_ };
        }

        [[nodiscard]] handle_type Handle() const noexcept {
            return handle_;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return handle_ != nullptr;
        }

    private:
        handle_type handle_;
    };

    template<>
    class [[nodiscard]] Task<void> {
    public:
        using promise_type = detail::TaskPromise<void>;
        using handle_type = std::coroutine_handle<promise_type>;
        using value_type = void;

        explicit Task(handle_type handle) noexcept
            : handle_(handle) {
        }

        Task(Task&& other) noexcept
            : handle_(std::exchange(other.handle_, nullptr)) {
        }

        Task& operator=(Task&& other) noexcept {
            if (this != &other) [[likely]] {
                if (handle_) {
                    handle_.destroy();
                }
                handle_ = std::exchange(other.handle_, nullptr);
            }
            return *this;
        }

        ~Task() {
            if (handle_) {
                handle_.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        [[nodiscard]] bool Resume() {
            if (!handle_ || handle_.done()) [[unlikely]] {
                return false;
            }
            handle_.resume();
            return !handle_.done();
        }

        [[nodiscard]] bool IsDone() const noexcept {
            return !handle_ || handle_.done();
        }

        [[nodiscard]] winsetup::domain::Result<> GetResult() const noexcept {
            if (!handle_) [[unlikely]] {
                return winsetup::domain::Result<>::Failure(
                    winsetup::domain::Error("Invalid task handle")
                );
            }
            if (!handle_.done()) [[unlikely]] {
                return winsetup::domain::Result<>::Failure(
                    winsetup::domain::Error("Task not completed")
                );
            }
            return handle_.promise().result();
        }

        void GetResultUnchecked() const noexcept {
        }

        void RunToCompletion() {
            while (Resume()) {
            }
        }

        auto operator co_await() const noexcept {
            struct Awaiter {
                handle_type handle;

                [[nodiscard]] bool await_ready() const noexcept {
                    return !handle || handle.done();
                }

                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<> continuation
                ) const noexcept {
                    return handle;
                }

                [[nodiscard]] winsetup::domain::Result<> await_resume() const noexcept {
                    if (!handle) [[unlikely]] {
                        return winsetup::domain::Result<>::Failure(
                            winsetup::domain::Error("Invalid task handle")
                        );
                    }
                    return handle.promise().result();
                }
            };
            return Awaiter{ handle_ };
        }

        [[nodiscard]] handle_type Handle() const noexcept {
            return handle_;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return handle_ != nullptr;
        }

    private:
        handle_type handle_;
    };

    namespace detail {
        template<typename T>
        Task<T> TaskPromise<T>::get_return_object() noexcept {
            return Task<T>(handle_type::from_promise(*this));
        }

        inline Task<void> TaskPromise<void>::get_return_object() noexcept {
            return Task<void>(handle_type::from_promise(*this));
        }
    }

}
