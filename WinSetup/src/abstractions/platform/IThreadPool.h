#pragma once

#include <cstddef>
#include <functional>

namespace winsetup::abstractions {

    enum class TaskPriority {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3
    };

    enum class TaskType {
        IOBound,
        CPUBound
    };

    class IThreadPool {
    public:
        virtual ~IThreadPool() = default;

        virtual void Submit(
            std::function<void()> task,
            TaskPriority priority = TaskPriority::Normal,
            TaskType type = TaskType::CPUBound
        ) = 0;

        virtual size_t GetThreadCount() const noexcept = 0;
        virtual size_t GetQueuedTaskCount() const noexcept = 0;
        virtual bool IsShutdown() const noexcept = 0;
        virtual void Shutdown() = 0;
        virtual void WaitForAll() = 0;
    };

}
