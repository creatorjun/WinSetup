#pragma once

#include <memory>
#include <chrono>
#include <vector>
#include <future>
#include <functional>
#include <type_traits>

namespace winsetup::abstractions {

    class IExecutor;
    class IScheduler;

    enum class SynchronizationContext {
        None,
        ThreadPool,
        UIThread,
        CurrentThread,
        Custom
    };

    class IAsyncContext {
    public:
        virtual ~IAsyncContext() = default;

        [[nodiscard]] virtual std::shared_ptr<IExecutor> GetExecutor() = 0;
        [[nodiscard]] virtual std::shared_ptr<IScheduler> GetScheduler() = 0;

        virtual void Post(std::function<void()> work) = 0;

        virtual void Send(std::function<void()> work) = 0;

        virtual void Invoke(std::function<void()> work) = 0;

        virtual void InvokeAsync(std::function<void()> work) = 0;

        [[nodiscard]] virtual SynchronizationContext GetSynchronizationContext() const = 0;

        virtual void SetSynchronizationContext(SynchronizationContext context) = 0;

        [[nodiscard]] virtual bool IsOnContextThread() const = 0;

        virtual void BeginInvoke(std::function<void()> work) = 0;

        virtual void RunOnContext(std::function<void()> work) = 0;

        template<typename TResult>
        [[nodiscard]] TResult RunOnContextAndWait(std::function<TResult()> work) {
            if (IsOnContextThread()) {
                return work();
            }

            using PromiseType = std::promise<TResult>;
            using FutureType = std::future<TResult>;

            PromiseType promiseObj;
            FutureType futureObj = promiseObj.get_future();

            Send([promisePtr = std::make_shared<PromiseType>(std::move(promiseObj)),
                workFunc = std::move(work)]() mutable {
                    try {
                        if constexpr (std::is_void_v<TResult>) {
                            workFunc();
                            promisePtr->set_value();
                        }
                        else {
                            promisePtr->set_value(workFunc());
                        }
                    }
                    catch (...) {
                        promisePtr->set_exception(std::current_exception());
                    }
                });

            return futureObj.get();
        }

        [[nodiscard]] virtual size_t GetPendingOperations() const = 0;

        virtual void ProcessPendingOperations() = 0;

        virtual void Shutdown() = 0;

        [[nodiscard]] virtual bool IsShutdown() const = 0;
    };

    class IAsyncContextFactory {
    public:
        virtual ~IAsyncContextFactory() = default;

        [[nodiscard]] virtual std::shared_ptr<IAsyncContext> CreateContext(
            SynchronizationContext contextType
        ) = 0;

        [[nodiscard]] virtual std::shared_ptr<IAsyncContext> GetCurrentContext() = 0;

        [[nodiscard]] virtual std::shared_ptr<IAsyncContext> GetDefaultContext() = 0;
    };

}
