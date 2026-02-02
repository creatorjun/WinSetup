#pragma once

#include <memory>
#include <functional>
#include "../../abstractions/async/IAsyncContext.h"
#include "TaskScheduler.h"
#include "Promise.h"
#include "Awaitable.h"

namespace winsetup::application {

    class AsyncContext {
    public:
        explicit AsyncContext(std::shared_ptr<TaskScheduler> scheduler)
            : scheduler_(std::move(scheduler)) {
        }

        template<typename T>
        Future<T> Run(std::function<T()> work) {
            Promise<T> promise;
            auto future = promise.GetFuture();

            scheduler_->Schedule([promise = std::move(promise), work = std::move(work)]() mutable {
                try {
                    if constexpr (std::is_void_v<T>) {
                        work();
                        promise.SetValue();
                    }
                    else {
                        promise.SetValue(work());
                    }
                }
                catch (const std::exception& e) {
                    promise.SetError(winsetup::domain::Error(e.what()));
                }
                catch (...) {
                    promise.SetError(winsetup::domain::Error("Unknown exception"));
                }
                });

            return future;
        }

        template<typename T>
        Future<T> RunOn(
            std::function<T()> work,
            abstractions::TaskPriority priority,
            abstractions::TaskType type
        ) {
            Promise<T> promise;
            auto future = promise.GetFuture();

            scheduler_->Schedule(
                [promise = std::move(promise), work = std::move(work)]() mutable {
                    try {
                        if constexpr (std::is_void_v<T>) {
                            work();
                            promise.SetValue();
                        }
                        else {
                            promise.SetValue(work());
                        }
                    }
                    catch (const std::exception& e) {
                        promise.SetError(winsetup::domain::Error(e.what()));
                    }
                    catch (...) {
                        promise.SetError(winsetup::domain::Error("Unknown exception"));
                    }
                },
                priority,
                type
            );

            return future;
        }

        std::shared_ptr<TaskScheduler> GetScheduler() const {
            return scheduler_;
        }

    private:
        std::shared_ptr<TaskScheduler> scheduler_;
    };

}
