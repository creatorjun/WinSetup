// src/abstractions/infrastructure/async/IThreadPool.h
#pragma once

#include <domain/primitives/Expected.h>
#include <functional>
#include <cstdint>
#include <future>

namespace winsetup::abstractions {

    enum class TaskPriority {
        Low,
        Normal,
        High,
        Critical
    };

    using TaskHandle = uint64_t;
    using TaskFunction = std::function<void()>;

    class IThreadPool {
    public:
        virtual ~IThreadPool() = default;

        [[nodiscard]] virtual domain::Expected<TaskHandle> Submit(
            TaskFunction task,
            TaskPriority priority = TaskPriority::Normal
        ) = 0;

        template<typename TResult>
        [[nodiscard]] domain::Expected<std::future<TResult>> SubmitWithResult(
            std::function<TResult()> task,
            TaskPriority priority = TaskPriority::Normal
        ) {
            auto promise = std::make_shared<std::promise<TResult>>();
            std::future<TResult> future = promise->get_future();

            auto wrapper = [task = std::move(task), promise]() mutable {
                try {
                    if constexpr (std::is_void_v<TResult>) {
                        task();
                        promise->set_value();
                    }
                    else {
                        promise->set_value(task());
                    }
                }
                catch (...) {
                    promise->set_exception(std::current_exception());
                }
                };

            auto result = Submit(std::move(wrapper), priority);
            if (!result.HasValue()) {
                return result.GetError();
            }

            return future;
        }

        [[nodiscard]] virtual bool Cancel(TaskHandle handle) = 0;
        virtual void CancelAll() = 0;

        [[nodiscard]] virtual size_t GetActiveThreadCount() const noexcept = 0;
        [[nodiscard]] virtual size_t GetQueuedTaskCount() const noexcept = 0;
        [[nodiscard]] virtual size_t GetCompletedTaskCount() const noexcept = 0;

        virtual void SetThreadCount(size_t count) = 0;
        virtual void WaitForAll() = 0;
    };

}
