#pragma once

#include <utility>
#include <type_traits>
#include <algorithm>

namespace winsetup::domain {

    template<typename T>
    struct DefaultDeleter {
        constexpr void operator()(T* ptr) const noexcept {
            delete ptr;
        }
    };

    template<typename T>
    struct DefaultDeleter<T[]> {
        constexpr void operator()(T* ptr) const noexcept {
            delete[] ptr;
        }
    };

    template<typename T, typename Deleter = DefaultDeleter<T>>
    class SmartPtr {
    public:
        using element_type = std::remove_extent_t<T>;
        using pointer = element_type*;
        using deleter_type = Deleter;

        constexpr explicit SmartPtr(pointer ptr = nullptr) noexcept
            : ptr_(ptr), deleter_() {
        }

        constexpr SmartPtr(pointer ptr, Deleter deleter) noexcept
            : ptr_(ptr), deleter_(std::move(deleter)) {
        }

        ~SmartPtr() {
            Reset();
        }

        SmartPtr(const SmartPtr&) = delete;
        SmartPtr& operator=(const SmartPtr&) = delete;

        constexpr SmartPtr(SmartPtr&& other) noexcept
            : ptr_(std::exchange(other.ptr_, nullptr))
            , deleter_(std::move(other.deleter_)) {
        }

        constexpr SmartPtr& operator=(SmartPtr&& other) noexcept {
            if (this != &other) [[likely]] {
                Reset();
                ptr_ = std::exchange(other.ptr_, nullptr);
                deleter_ = std::move(other.deleter_);
            }
            return *this;
        }

        [[nodiscard]] constexpr pointer Get() const noexcept {
            return ptr_;
        }

        [[nodiscard]] constexpr pointer Release() noexcept {
            return std::exchange(ptr_, nullptr);
        }

        constexpr void Reset(pointer ptr = nullptr) {
            if (ptr_) [[likely]] {
                deleter_(ptr_);
            }
            ptr_ = ptr;
        }

        template<typename U = T>
        [[nodiscard]] constexpr std::enable_if_t<!std::is_array_v<U>, element_type&>
            operator*() const {
            return *ptr_;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept {
            return ptr_;
        }

        template<typename U = T>
        [[nodiscard]] constexpr std::enable_if_t<std::is_array_v<U>, element_type&>
            operator[](size_t index) const {
            return ptr_[index];
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept {
            return ptr_ != nullptr;
        }

        constexpr void Swap(SmartPtr& other) noexcept {
            std::swap(ptr_, other.ptr_);
            std::swap(deleter_, other.deleter_);
        }

        [[nodiscard]] constexpr Deleter& GetDeleter() noexcept {
            return deleter_;
        }

        [[nodiscard]] constexpr const Deleter& GetDeleter() const noexcept {
            return deleter_;
        }

    private:
        pointer ptr_;
        [[no_unique_address]] Deleter deleter_;
    };

    template<typename T, typename Deleter>
    constexpr void swap(SmartPtr<T, Deleter>& lhs, SmartPtr<T, Deleter>& rhs) noexcept {
        lhs.Swap(rhs);
    }

    template<typename T, typename... Args>
    [[nodiscard]] constexpr std::enable_if_t<!std::is_array_v<T>, SmartPtr<T>>
        MakeSmartPtr(Args&&... args) {
        return SmartPtr<T>(new T(std::forward<Args>(args)...));
    }

    template<typename T>
    [[nodiscard]] constexpr std::enable_if_t<std::is_array_v<T>&& std::extent_v<T> == 0, SmartPtr<T>>
        MakeSmartPtr(size_t size) {
        using ElementType = std::remove_extent_t<T>;
        return SmartPtr<T>(new ElementType[size]());
    }

}
