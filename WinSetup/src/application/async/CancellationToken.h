#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>

namespace winsetup::application {

    class CancellationToken;

    class CancellationSource {
    public:
        CancellationSource()
            : cancelled_(std::make_shared<std::atomic<bool>>(false))
            , callbacks_(std::make_shared<CallbackList>()) {
        }

        void Cancel() {
            bool expected = false;
            if (cancelled_->compare_exchange_strong(expected, true)) {
                InvokeCallbacks();
            }
        }

        bool IsCancelled() const noexcept {
            return cancelled_->load(std::memory_order_acquire);
        }

        CancellationToken GetToken() const noexcept;

    private:
        struct CallbackList {
            std::mutex mutex;
            std::vector<std::function<void()>> callbacks;
        };

        void InvokeCallbacks() {
            std::vector<std::function<void()>> localCallbacks;
            {
                std::lock_guard<std::mutex> lock(callbacks_->mutex);
                localCallbacks = std::move(callbacks_->callbacks);
            }
            for (auto& callback : localCallbacks) {
                if (callback) {
                    callback();
                }
            }
        }

        std::shared_ptr<std::atomic<bool>> cancelled_;
        std::shared_ptr<CallbackList> callbacks_;

        friend class CancellationToken;
    };

    class CancellationToken {
    public:
        CancellationToken() = default;

        bool IsCancelled() const noexcept {
            return cancelled_ && cancelled_->load(std::memory_order_acquire);
        }

        void ThrowIfCancelled() const {
            if (IsCancelled()) {
                throw std::runtime_error("Operation was cancelled");
            }
        }

        void Register(std::function<void()> callback) {
            if (!callbacks_) return;

            bool shouldExecute = false;
            {
                std::lock_guard<std::mutex> lock(callbacks_->mutex);
                if (IsCancelled()) {
                    shouldExecute = true;
                }
                else {
                    callbacks_->callbacks.push_back(std::move(callback));
                }
            }

            if (shouldExecute && callback) {
                callback();
            }
        }

        explicit operator bool() const noexcept {
            return cancelled_ != nullptr;
        }

    private:
        CancellationToken(
            std::shared_ptr<std::atomic<bool>> cancelled,
            std::shared_ptr<CancellationSource::CallbackList> callbacks
        ) : cancelled_(cancelled), callbacks_(callbacks) {
        }

        std::shared_ptr<std::atomic<bool>> cancelled_;
        std::shared_ptr<CancellationSource::CallbackList> callbacks_;

        friend class CancellationSource;
    };

    inline CancellationToken CancellationSource::GetToken() const noexcept {
        return CancellationToken(cancelled_, callbacks_);
    }

}
