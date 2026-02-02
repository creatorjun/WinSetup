#include "TaskScheduler.h"
#include <algorithm>

namespace winsetup::application {

    TaskScheduler::TaskScheduler(std::shared_ptr<abstractions::IThreadPool> threadPool)
        : threadPool_(std::move(threadPool))
        , nextTaskId_(1)
        , shutdown_(false) {
    }

    TaskScheduler::~TaskScheduler() {
        Shutdown();
    }

    TaskId TaskScheduler::Schedule(
        std::function<void()> work,
        abstractions::TaskPriority priority,
        abstractions::TaskType type
    ) {
        return Schedule(std::move(work), {}, priority, type);
    }

    TaskId TaskScheduler::Schedule(
        std::function<void()> work,
        const std::vector<TaskId>& dependencies,
        abstractions::TaskPriority priority,
        abstractions::TaskType type
    ) {
        return ScheduleWithCancellation(
            std::move(work),
            dependencies,
            CancellationToken(),
            priority,
            type
        );
    }

    TaskId TaskScheduler::ScheduleWithCancellation(
        std::function<void()> work,
        CancellationToken token,
        abstractions::TaskPriority priority,
        abstractions::TaskType type
    ) {
        return ScheduleWithCancellation(
            std::move(work),
            {},
            std::move(token),
            priority,
            type
        );
    }

    TaskId TaskScheduler::ScheduleWithCancellation(
        std::function<void()> work,
        const std::vector<TaskId>& dependencies,
        CancellationToken token,
        abstractions::TaskPriority priority,
        abstractions::TaskType type
    ) {
        if (shutdown_.load(std::memory_order_acquire)) {
            throw std::runtime_error("TaskScheduler is shut down");
        }

        TaskId taskId = GenerateTaskId();
        auto taskState = std::make_shared<TaskState>();
        taskState->descriptor.id = taskId;
        taskState->descriptor.work = std::move(work);
        taskState->descriptor.priority = priority;
        taskState->descriptor.type = type;
        taskState->descriptor.dependencies = dependencies;
        taskState->descriptor.cancellationToken = std::move(token);
        taskState->remainingDependencies.store(dependencies.size(), std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_[taskId] = taskState;

            for (TaskId depId : dependencies) {
                auto it = tasks_.find(depId);
                if (it != tasks_.end()) {
                    it->second->dependents.insert(taskId);
                }
            }

            if (dependencies.empty()) {
                EnqueueReadyTask(taskState);
            }
        }

        return taskId;
    }

    void TaskScheduler::WaitForTask(TaskId taskId) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this, taskId]() {
            auto it = tasks_.find(taskId);
            return it == tasks_.end() || it->second->completed.load(std::memory_order_acquire);
            });
    }

    void TaskScheduler::WaitForAll() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return std::all_of(tasks_.begin(), tasks_.end(), [](const auto& pair) {
                return pair.second->completed.load(std::memory_order_acquire);
                });
            });
    }

    void TaskScheduler::CancelTask(TaskId taskId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(taskId);
        if (it != tasks_.end()) {
            it->second->cancelled.store(true, std::memory_order_release);
        }
    }

    bool TaskScheduler::IsTaskCompleted(TaskId taskId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(taskId);
        if (it != tasks_.end()) {
            return it->second->completed.load(std::memory_order_acquire);
        }
        return false;
    }

    size_t TaskScheduler::GetPendingTaskCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::count_if(tasks_.begin(), tasks_.end(), [](const auto& pair) {
            return !pair.second->completed.load(std::memory_order_acquire);
            });
    }

    void TaskScheduler::Shutdown() {
        bool expected = false;
        if (shutdown_.compare_exchange_strong(expected, true)) {
            WaitForAll();
            if (threadPool_) {
                threadPool_->Shutdown();
            }
        }
    }

    void TaskScheduler::ExecuteTask(std::shared_ptr<TaskState> taskState) {
        if (taskState->cancelled.load(std::memory_order_acquire)) {
            taskState->completed.store(true, std::memory_order_release);
            cv_.notify_all();
            ProcessDependents(taskState->descriptor.id);
            return;
        }

        if (taskState->descriptor.cancellationToken &&
            taskState->descriptor.cancellationToken.IsCancelled()) {
            taskState->cancelled.store(true, std::memory_order_release);
            taskState->completed.store(true, std::memory_order_release);
            cv_.notify_all();
            ProcessDependents(taskState->descriptor.id);
            return;
        }

        try {
            if (taskState->descriptor.work) {
                taskState->descriptor.work();
            }
        }
        catch (...) {
        }

        taskState->completed.store(true, std::memory_order_release);
        cv_.notify_all();
        ProcessDependents(taskState->descriptor.id);
    }

    void TaskScheduler::ProcessDependents(TaskId completedTaskId) {
        std::vector<std::shared_ptr<TaskState>> readyTasks;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tasks_.find(completedTaskId);
            if (it == tasks_.end()) {
                return;
            }

            for (TaskId dependentId : it->second->dependents) {
                auto depIt = tasks_.find(dependentId);
                if (depIt != tasks_.end()) {
                    size_t remaining = depIt->second->remainingDependencies.fetch_sub(
                        1, std::memory_order_acq_rel
                    ) - 1;

                    if (remaining == 0) {
                        readyTasks.push_back(depIt->second);
                    }
                }
            }
        }

        for (auto& taskState : readyTasks) {
            EnqueueReadyTask(taskState);
        }
    }

    void TaskScheduler::EnqueueReadyTask(std::shared_ptr<TaskState> taskState) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        threadPool_->Submit(
            [this, taskState]() {
                ExecuteTask(taskState);
            },
            taskState->descriptor.priority,
            taskState->descriptor.type
        );
    }

    TaskId TaskScheduler::GenerateTaskId() {
        return nextTaskId_.fetch_add(1, std::memory_order_relaxed);
    }

}
