// src/domain/memory/UniqueResource.h

#pragma once

#include <utility>
#include <functional>

namespace winsetup::domain {

    template<typename T, typename Deleter = std::function<void(T)>>
    class UniqueResource {
    public:
        UniqueResource(T resource, Deleter deleter)
            : resource_(std::move(resource))
            , deleter_(std::move(deleter))
            , owns_(true) {
        }

        ~UniqueResource() {
            Reset();
        }

        UniqueResource(const UniqueResource&) = delete;
        UniqueResource& operator=(const UniqueResource&) = delete;

        UniqueResource(UniqueResource&& other) noexcept
            : resource_(std::move(other.resource_))
            , deleter_(std::move(other.deleter_))
            , owns_(other.owns_) {
            other.owns_ = false;
        }

        UniqueResource& operator=(UniqueResource&& other) noexcept {
            if (this != &other) {
                Reset();
                resource_ = std::move(other.resource_);
                deleter_ = std::move(other.deleter_);
                owns_ = other.owns_;
                other.owns_ = false;
            }
            return *this;
        }

        T Get() const noexcept {
            return resource_;
        }

        T Release() noexcept {
            owns_ = false;
            return resource_;
        }

        void Reset(T newResource) {
            if (owns_) {
                deleter_(resource_);
            }
            resource_ = std::move(newResource);
            owns_ = true;
        }

        void Reset() {
            if (owns_) {
                deleter_(resource_);
                owns_ = false;
            }
        }

        explicit operator bool() const noexcept {
            return owns_;
        }

        const Deleter& GetDeleter() const noexcept {
            return deleter_;
        }

    private:
        T resource_;
        Deleter deleter_;
        bool owns_;
    };

    template<typename T, typename Deleter>
    UniqueResource<T, Deleter> MakeUniqueResource(T resource, Deleter deleter) {
        return UniqueResource<T, Deleter>(std::move(resource), std::move(deleter));
    }

}
