// src/abstractions/async/IExecutor.h 
#pragma once

#include <chrono>
#include <future>
#include <functional>

namespace winsetup::abstractions {

    enum class ExecutionPolicy {
        Inline,
        Deferred,
        Async,
        Parallel
    };

    class IExecutor {
    public:
        virtual ~IExecutor() = default;

        virtual void Execute(std::function<void()> work) = 0;

        virtual void ExecuteDelayed(
            std::function<void()> work,
            std::chrono::milliseconds delay
        ) = 0;

        virtual std::future<void> ExecuteAsync(
            std::function<void()> work
        ) = 0;

        template<typename TResult>
        std::future<TResult> ExecuteAsync(std::function<TResult()> work) {
            return ExecuteAsyncImpl<TResult>(std::move(work));
        }

        virtual void ExecuteWithPolicy(
            std::function<void()> work,
            ExecutionPolicy policy
        ) = 0;

        [[nodiscard]] virtual bool IsIdle() const noexcept = 0;
        [[nodiscard]] virtual size_t GetPendingWorkCount() const noexcept = 0;

        virtual void Shutdown() = 0;
        virtual void WaitForCompletion() = 0;

    protected:
        template<typename TResult>
        std::future<TResult> ExecuteAsyncImpl(std::function<TResult()> work) {
            auto promise = std::make_shared<std::promise<TResult>>();
            auto future = promise->get_future();

            Execute([work = std::move(work), promise]() mutable {
                try {
                    if constexpr (std::is_void_v<TResult>) {
                        work();
                        promise->set_value();
                    }
                    else {
                        promise->set_value(work());
                    }
                }
                catch (...) {
                    promise->set_exception(std::current_exception());
                }
                });

            return future;
        }
    };

}
