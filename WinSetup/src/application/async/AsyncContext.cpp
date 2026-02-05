#include "AsyncContext.h"

namespace winsetup::application {

    AsyncContext::AsyncContext(
        std::shared_ptr<abstractions::IExecutor> executor,
        std::shared_ptr<abstractions::IScheduler> scheduler
    )
        : executor_(std::move(executor))
        , scheduler_(std::move(scheduler))
        , syncContext_(abstractions::SynchronizationContext::ThreadPool)
        , contextThreadId_(std::this_thread::get_id()) {
    }

    AsyncContext::~AsyncContext() {
        Shutdown();
    }

    std::shared_ptr<abstractions::IExecutor> AsyncContext::GetExecutor() {
        return executor_;
    }

    std::shared_ptr<abstractions::IScheduler> AsyncContext::GetScheduler() {
        return scheduler_;
    }

    void AsyncContext::Post(std::function<void()> work) {
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            operationQueue_.push(Operation{ std::move(work), false, nullptr });
        }
        queueCv_.notify_one();
    }

    void AsyncContext::Send(std::function<void()> work) {
        if (IsOnContextThread()) {
            work();
            return;
        }

        if (shutdown_.load(std::memory_order_acquire)) {
            throw std::runtime_error("Context is shutdown");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            operationQueue_.push(Operation{ std::move(work), true, promise });
        }
        queueCv_.notify_one();

        future.get();
    }

    void AsyncContext::Invoke(std::function<void()> work) {
        Send(std::move(work));
    }

    void AsyncContext::InvokeAsync(std::function<void()> work) {
        Post(std::move(work));
    }

    abstractions::SynchronizationContext AsyncContext::GetSynchronizationContext() const {
        return syncContext_;
    }

    void AsyncContext::SetSynchronizationContext(abstractions::SynchronizationContext context) {
        syncContext_ = context;
    }

    bool AsyncContext::IsOnContextThread() const {
        return std::this_thread::get_id() == contextThreadId_;
    }

    void AsyncContext::BeginInvoke(std::function<void()> work) {
        Post(std::move(work));
    }

    void AsyncContext::RunOnContext(std::function<void()> work) {
        if (IsOnContextThread()) {
            work();
        }
        else {
            Send(std::move(work));
        }
    }

    size_t AsyncContext::GetPendingOperations() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return operationQueue_.size();
    }

    void AsyncContext::ProcessPendingOperations() {
        std::vector<Operation> operations;

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            while (!operationQueue_.empty()) {
                operations.push_back(std::move(operationQueue_.front()));
                operationQueue_.pop();
            }
        }

        for (auto& op : operations) {
            if (shutdown_.load(std::memory_order_acquire)) {
                if (op.promise) {
                    op.promise->set_exception(
                        std::make_exception_ptr(std::runtime_error("Context is shutdown"))
                    );
                }
                break;
            }

            try {
                op.work();
                if (op.promise) {
                    op.promise->set_value();
                }
            }
            catch (...) {
                if (op.promise) {
                    op.promise->set_exception(std::current_exception());
                }
            }
        }
    }

    void AsyncContext::Shutdown() {
        bool expected = false;
        if (shutdown_.compare_exchange_strong(expected, true)) {
            queueCv_.notify_all();

            if (executor_) {
                executor_->Shutdown();
            }
            if (scheduler_) {
                scheduler_->Shutdown();
            }
        }
    }

    bool AsyncContext::IsShutdown() const {
        return shutdown_.load(std::memory_order_acquire);
    }

}
