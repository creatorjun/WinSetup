#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <abstractions/async/IExecutor.h>
#include <abstractions/async/IScheduler.h>
#include <abstractions/async/IAsyncContext.h>

namespace winsetup::application {

    class AsyncContext : public abstractions::IAsyncContext {
    public:
        explicit AsyncContext(
            std::shared_ptr<abstractions::IExecutor> executor,
            std::shared_ptr<abstractions::IScheduler> scheduler
        );

        ~AsyncContext() override;

        AsyncContext(const AsyncContext&) = delete;
        AsyncContext& operator=(const AsyncContext&) = delete;

        [[nodiscard]] std::shared_ptr<abstractions::IExecutor> GetExecutor() override;
        [[nodiscard]] std::shared_ptr<abstractions::IScheduler> GetScheduler() override;

        void Post(std::function<void()> work) override;
        void Send(std::function<void()> work) override;
        void Invoke(std::function<void()> work) override;
        void InvokeAsync(std::function<void()> work) override;

        [[nodiscard]] abstractions::SynchronizationContext GetSynchronizationContext() const override;
        void SetSynchronizationContext(abstractions::SynchronizationContext context) override;

        [[nodiscard]] bool IsOnContextThread() const override;

        void BeginInvoke(std::function<void()> work) override;
        void RunOnContext(std::function<void()> work) override;

        [[nodiscard]] size_t GetPendingOperations() const override;
        void ProcessPendingOperations() override;

        void Shutdown() override;
        [[nodiscard]] bool IsShutdown() const override;

    private:
        struct Operation {
            std::function<void()> work;
            bool isBlocking{ false };
            std::shared_ptr<std::promise<void>> promise;
        };

        std::shared_ptr<abstractions::IExecutor> executor_;
        std::shared_ptr<abstractions::IScheduler> scheduler_;
        abstractions::SynchronizationContext syncContext_;
        std::thread::id contextThreadId_;
        std::atomic<bool> shutdown_{ false };

        mutable std::mutex queueMutex_;
        std::queue<Operation> operationQueue_;
        std::condition_variable queueCv_;
    };

}
