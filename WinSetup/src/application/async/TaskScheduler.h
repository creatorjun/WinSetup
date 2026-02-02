#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include "../../abstractions/platform/IThreadPool.h"
#include "CancellationToken.h"
#include "Task.h"

namespace winsetup::application {

    using TaskId = size_t;

    struct TaskDescriptor {
        TaskId id;
        std::function<void()> work;
        abstractions::TaskPriority priority;
        abstractions::TaskType type;
        std::vector<TaskId> dependencies;
        CancellationToken cancellationToken;

        TaskDescriptor()
            : id(0)
            , priority(abstractions::TaskPriority::Normal)
            , type(abstractions::TaskType::CPUBound) {
        }
    };

    class TaskScheduler {
    public:
        explicit TaskScheduler(std::shared_ptr<abstractions::IThreadPool> threadPool);
        ~TaskScheduler();

        TaskScheduler(const TaskScheduler&) = delete;
        TaskScheduler& operator=(const TaskScheduler&) = delete;

        TaskId Schedule(
            std::function<void()> work,
            abstractions::TaskPriority priority = abstractions::TaskPriority::Normal,
            abstractions::TaskType type = abstractions::TaskType::CPUBound
        );

        TaskId Schedule(
            std::function<void()> work,
            const std::vector<TaskId>& dependencies,
            abstractions::TaskPriority priority = abstractions::TaskPriority::Normal,
            abstractions::TaskType type = abstractions::TaskType::CPUBound
        );

        TaskId ScheduleWithCancellation(
            std::function<void()> work,
            CancellationToken token,
            abstractions::TaskPriority priority = abstractions::TaskPriority::Normal,
            abstractions::TaskType type = abstractions::TaskType::CPUBound
        );

        TaskId ScheduleWithCancellation(
            std::function<void()> work,
            const std::vector<TaskId>& dependencies,
            CancellationToken token,
            abstractions::TaskPriority priority = abstractions::TaskPriority::Normal,
            abstractions::TaskType type = abstractions::TaskType::CPUBound
        );

        void WaitForTask(TaskId taskId);
        void WaitForAll();
        void CancelTask(TaskId taskId);
        bool IsTaskCompleted(TaskId taskId) const;
        size_t GetPendingTaskCount() const;
        void Shutdown();

    private:
        struct TaskState {
            TaskDescriptor descriptor;
            std::atomic<bool> completed;
            std::atomic<bool> cancelled;
            std::unordered_set<TaskId> dependents;
            std::atomic<size_t> remainingDependencies;

            TaskState() : completed(false), cancelled(false), remainingDependencies(0) {}
        };

        struct TaskCompare {
            bool operator()(const std::shared_ptr<TaskState>& a, const std::shared_ptr<TaskState>& b) const {
                return static_cast<int>(a->descriptor.priority) < static_cast<int>(b->descriptor.priority);
            }
        };

        void ExecuteTask(std::shared_ptr<TaskState> taskState);
        void ProcessDependents(TaskId completedTaskId);
        void EnqueueReadyTask(std::shared_ptr<TaskState> taskState);
        TaskId GenerateTaskId();

        std::shared_ptr<abstractions::IThreadPool> threadPool_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;

        std::unordered_map<TaskId, std::shared_ptr<TaskState>> tasks_;
        std::priority_queue<
            std::shared_ptr<TaskState>,
            std::vector<std::shared_ptr<TaskState>>,
            TaskCompare
        > readyQueue_;

        std::atomic<TaskId> nextTaskId_;
        std::atomic<bool> shutdown_;
    };

}
