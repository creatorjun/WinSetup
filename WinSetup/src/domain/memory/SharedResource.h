// src/domain/memory/SharedResource.h

#pragma once

#include <atomic>
#include <utility>
#include <functional>

namespace winsetup::domain {

    template<typename T, typename Deleter = std::function<void(T)>>
    class SharedResource {
    private:
        struct ControlBlock {
            T resource;
            Deleter deleter;
            std::atomic<size_t> refCount;

            ControlBlock(T res, Deleter del)
                : resource(std::move(res))
                , deleter(std::move(del))
                , refCount(1) {
            }

            ~ControlBlock() {
                deleter(resource);
            }
        };

    public:
        SharedResource(T resource, Deleter deleter)
            : control_(new ControlBlock(std::move(resource), std::move(deleter))) {
        }

        ~SharedResource() {
            Release();
        }

        SharedResource(const SharedResource& other) noexcept
            : control_(other.control_) {
            if (control_) {
                control_->refCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        SharedResource& operator=(const SharedResource& other) noexcept {
            if (this != &other) {
                Release();
                control_ = other.control_;
                if (control_) {
                    control_->refCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
            return *this;
        }

        SharedResource(SharedResource&& other) noexcept
            : control_(other.control_) {
            other.control_ = nullptr;
        }

        SharedResource& operator=(SharedResource&& other) noexcept {
            if (this != &other) {
                Release();
                control_ = other.control_;
                other.control_ = nullptr;
            }
            return *this;
        }

        T Get() const noexcept {
            return control_ ? control_->resource : T{};
        }

        size_t UseCount() const noexcept {
            return control_ ? control_->refCount.load(std::memory_order_relaxed) : 0;
        }

        bool IsUnique() const noexcept {
            return UseCount() == 1;
        }

        explicit operator bool() const noexcept {
            return control_ != nullptr;
        }

        const Deleter& GetDeleter() const {
            if (!control_) {
                throw std::logic_error("SharedResource is empty");
            }
            return control_->deleter;
        }

        void Reset() {
            Release();
            control_ = nullptr;
        }

    private:
        void Release() {
            if (control_) {
                if (control_->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    delete control_;
                }
            }
        }

        ControlBlock* control_;
    };

    template<typename T, typename Deleter>
    SharedResource<T, Deleter> MakeSharedResource(T resource, Deleter deleter) {
        return SharedResource<T, Deleter>(std::move(resource), std::move(deleter));
    }

}
