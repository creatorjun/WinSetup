#pragma once

#include <utility>
#include <type_traits>

namespace winsetup::domain {

    template<typename T>
    struct DefaultDeleter {
        void operator()(T* ptr) const {
            delete ptr;
        }
    };

    template<typename T>
    struct DefaultDeleter<T[]> {
        void operator()(T* ptr) const {
            delete[] ptr;
        }
    };

    template<typename T, typename Deleter = DefaultDeleter<T>>
    class SmartPtr {
    public:
        using element_type = std::remove_extent_t<T>;
        using pointer = element_type*;

        explicit SmartPtr(pointer ptr = nullptr) noexcept
            : ptr_(ptr), deleter_() {
        }

        SmartPtr(pointer ptr, Deleter deleter) noexcept
            : ptr_(ptr), deleter_(std::move(deleter)) {
        }

        ~SmartPtr() {
            Reset();
        }

        SmartPtr(const SmartPtr&) = delete;
        SmartPtr& operator=(const SmartPtr&) = delete;

        SmartPtr(SmartPtr&& other) noexcept
            : ptr_(other.ptr_), deleter_(std::move(other.deleter_)) {
            other.ptr_ = nullptr;
        }

        SmartPtr& operator=(SmartPtr&& other) noexcept {
            if (this != &other) {
                Reset();
                ptr_ = other.ptr_;
                deleter_ = std::move(other.deleter_);
                other.ptr_ = nullptr;
            }
            return *this;
        }

        pointer Get() const noexcept {
            return ptr_;
        }

        pointer Release() noexcept {
            pointer temp = ptr_;
            ptr_ = nullptr;
            return temp;
        }

        void Reset(pointer ptr = nullptr) {
            if (ptr_) {
                deleter_(ptr_);
            }
            ptr_ = ptr;
        }

        template<typename U = T>
        std::enable_if_t<!std::is_array_v<U>, element_type&>
            operator*() const {
            return *ptr_;
        }

        pointer operator->() const noexcept {
            return ptr_;
        }

        template<typename U = T>
        std::enable_if_t<std::is_array_v<U>, element_type&>
            operator[](size_t index) const {
            return ptr_[index];
        }

        explicit operator bool() const noexcept {
            return ptr_ != nullptr;
        }

        void Swap(SmartPtr& other) noexcept {
            pointer tempPtr = ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = tempPtr;

            Deleter tempDeleter = std::move(deleter_);
            deleter_ = std::move(other.deleter_);
            other.deleter_ = std::move(tempDeleter);
        }

        Deleter& GetDeleter() noexcept {
            return deleter_;
        }

        const Deleter& GetDeleter() const noexcept {
            return deleter_;
        }

    private:
        pointer ptr_;
        Deleter deleter_;
    };

    template<typename T, typename Deleter>
    void swap(SmartPtr<T, Deleter>& lhs, SmartPtr<T, Deleter>& rhs) noexcept {
        lhs.Swap(rhs);
    }

    template<typename T, typename... Args>
    std::enable_if_t<!std::is_array_v<T>, SmartPtr<T>>
        MakeSmartPtr(Args&&... args) {
        return SmartPtr<T>(new T(std::forward<Args>(args)...));
    }

    template<typename T>
    std::enable_if_t<std::is_array_v<T>&& std::extent_v<T> == 0, SmartPtr<T>>
        MakeSmartPtr(size_t size) {
        using ElementType = std::remove_extent_t<T>;
        return SmartPtr<T>(new ElementType[size]());
    }

}
