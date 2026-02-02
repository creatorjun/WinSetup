// src/domain/memory/SmartPtr.h

#pragma once

#include <utility>
#include <type_traits>

namespace winsetup::domain {

    template<typename T>
    class SmartPtr {
    public:
        explicit SmartPtr(T* ptr = nullptr) noexcept
            : ptr_(ptr) {
        }

        ~SmartPtr() {
            Reset();
        }

        SmartPtr(const SmartPtr&) = delete;
        SmartPtr& operator=(const SmartPtr&) = delete;

        SmartPtr(SmartPtr&& other) noexcept
            : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        SmartPtr& operator=(SmartPtr&& other) noexcept {
            if (this != &other) {
                Reset();
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        T* Get() const noexcept {
            return ptr_;
        }

        T* Release() noexcept {
            T* temp = ptr_;
            ptr_ = nullptr;
            return temp;
        }

        void Reset(T* ptr = nullptr) {
            if (ptr_) {
                delete ptr_;
            }
            ptr_ = ptr;
        }

        T& operator*() const {
            return *ptr_;
        }

        T* operator->() const noexcept {
            return ptr_;
        }

        explicit operator bool() const noexcept {
            return ptr_ != nullptr;
        }

        void Swap(SmartPtr& other) noexcept {
            T* temp = ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = temp;
        }

    private:
        T* ptr_;
    };

    template<typename T>
    void swap(SmartPtr<T>& lhs, SmartPtr<T>& rhs) noexcept {
        lhs.Swap(rhs);
    }

    template<typename T, typename... Args>
    SmartPtr<T> MakeSmartPtr(Args&&... args) {
        return SmartPtr<T>(new T(std::forward<Args>(args)...));
    }

}
