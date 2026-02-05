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
            : control_(std::make_shared<ControlBlock>()) {
        }

        void Cancel() {
            bool expected = false;
            if (control_->cancelled.compare_exchange_strong(expected, true)) [[likely]] {
                InvokeCallbacks();
            }
        }

        [[nodiscard]] bool IsCancelled() const noexcept {
            return control_->cancelled.load(std::memory_order_acquire);
        }

        [[nodiscard]] CancellationToken GetToken() const noexcept;

    private:
        struct ControlBlock {
            std::atomic<bool> cancelled{ false };
            std::mutex mutex;
            std::vector<std::function<void()>> callbacks;
        };

        void InvokeCallbacks() {
            std::vector<std::function<void()>> localCallbacks;
            {
                std::lock_guard<std::mutex> lock(control_->mutex);
                localCallbacks = std::move(control_->callbacks);
            }
            for (auto& callback : localCallbacks) {
                if (callback) [[likely]] {
                    callback();
                }
            }
        }

        std::shared_ptr<ControlBlock> control_;

        friend class CancellationToken;
    };

    class CancellationToken {
    public:
        CancellationToken() = default;

        [[nodiscard]] bool IsCancelled() const noexcept {
            return control_ && control_->cancelled.load(std::memory_order_acquire);
        }

        void ThrowIfCancelled() const {
            if (IsCancelled()) [[unlikely]] {
                throw std::runtime_error("Operation was cancelled");
            }
        }

        void Register(std::function<void()> callback) {
            if (!control_) [[unlikely]] return;

            bool shouldExecute = false;
            {
                std::lock_guard<std::mutex> lock(control_->mutex);
                if (IsCancelled()) {
                    shouldExecute = true;
                }
                else {
                    control_->callbacks.push_back(std::move(callback));
                }
            }

            if (shouldExecute && callback) [[unlikely]] {
                callback();
            }
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return control_ != nullptr;
        }

    private:
        explicit CancellationToken(std::shared_ptr<CancellationSource::ControlBlock> control)
            : control_(std::move(control)) {
        }

        std::shared_ptr<CancellationSource::ControlBlock> control_;

        friend class CancellationSource;
    };

    inline CancellationToken CancellationSource::GetToken() const noexcept {
        return CancellationToken(control_);
    }

}
