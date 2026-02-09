// src/domain/memory/UniqueHandle.h
#pragma once

#include <utility>
#include <cstdint>

namespace winsetup::domain {

    using NativeHandle = void*;

    inline constexpr NativeHandle InvalidHandleValue() noexcept {
        return reinterpret_cast<NativeHandle>(static_cast<intptr_t>(-1));
    }

    using HandleDeleterFunc = void(*)(NativeHandle) noexcept;

    class UniqueHandle {
    public:
        UniqueHandle() noexcept
            : m_handle(InvalidHandleValue())
            , m_deleter(nullptr)
        {
        }

        explicit UniqueHandle(NativeHandle handle, HandleDeleterFunc deleter) noexcept
            : m_handle(handle)
            , m_deleter(deleter)
        {
        }

        ~UniqueHandle() noexcept {
            Close();
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : m_handle(std::exchange(other.m_handle, InvalidHandleValue()))
            , m_deleter(std::exchange(other.m_deleter, nullptr))
        {
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, InvalidHandleValue());
                m_deleter = std::exchange(other.m_deleter, nullptr);
            }
            return *this;
        }

        [[nodiscard]] NativeHandle Get() const noexcept {
            return m_handle;
        }

        [[nodiscard]] NativeHandle Release() noexcept {
            m_deleter = nullptr;
            return std::exchange(m_handle, InvalidHandleValue());
        }

        void Reset(NativeHandle handle = InvalidHandleValue(), HandleDeleterFunc deleter = nullptr) noexcept {
            Close();
            m_handle = handle;
            m_deleter = deleter;
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_handle != InvalidHandleValue() && m_handle != nullptr;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return IsValid();
        }

        [[nodiscard]] NativeHandle* AddressOf() noexcept {
            return &m_handle;
        }

    private:
        void Close() noexcept {
            if (IsValid() && m_deleter) {
                m_deleter(m_handle);
            }
        }

        NativeHandle m_handle;
        HandleDeleterFunc m_deleter;
    };

    class UniqueLibrary {
    public:
        UniqueLibrary() noexcept
            : m_handle(nullptr)
            , m_deleter(nullptr)
        {
        }

        explicit UniqueLibrary(NativeHandle handle, HandleDeleterFunc deleter) noexcept
            : m_handle(handle)
            , m_deleter(deleter)
        {
        }

        ~UniqueLibrary() noexcept {
            Close();
        }

        UniqueLibrary(const UniqueLibrary&) = delete;
        UniqueLibrary& operator=(const UniqueLibrary&) = delete;

        UniqueLibrary(UniqueLibrary&& other) noexcept
            : m_handle(std::exchange(other.m_handle, nullptr))
            , m_deleter(std::exchange(other.m_deleter, nullptr))
        {
        }

        UniqueLibrary& operator=(UniqueLibrary&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, nullptr);
                m_deleter = std::exchange(other.m_deleter, nullptr);
            }
            return *this;
        }

        [[nodiscard]] NativeHandle Get() const noexcept {
            return m_handle;
        }

        [[nodiscard]] NativeHandle Release() noexcept {
            m_deleter = nullptr;
            return std::exchange(m_handle, nullptr);
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_handle != nullptr;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return IsValid();
        }

    private:
        void Close() noexcept {
            if (IsValid() && m_deleter) {
                m_deleter(m_handle);
            }
        }

        NativeHandle m_handle;
        HandleDeleterFunc m_deleter;
    };

    class UniqueFindHandle {
    public:
        UniqueFindHandle() noexcept
            : m_handle(InvalidHandleValue())
            , m_deleter(nullptr)
        {
        }

        explicit UniqueFindHandle(NativeHandle handle, HandleDeleterFunc deleter) noexcept
            : m_handle(handle)
            , m_deleter(deleter)
        {
        }

        ~UniqueFindHandle() noexcept {
            Close();
        }

        UniqueFindHandle(const UniqueFindHandle&) = delete;
        UniqueFindHandle& operator=(const UniqueFindHandle&) = delete;

        UniqueFindHandle(UniqueFindHandle&& other) noexcept
            : m_handle(std::exchange(other.m_handle, InvalidHandleValue()))
            , m_deleter(std::exchange(other.m_deleter, nullptr))
        {
        }

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, InvalidHandleValue());
                m_deleter = std::exchange(other.m_deleter, nullptr);
            }
            return *this;
        }

        [[nodiscard]] NativeHandle Get() const noexcept {
            return m_handle;
        }

        [[nodiscard]] NativeHandle Release() noexcept {
            m_deleter = nullptr;
            return std::exchange(m_handle, InvalidHandleValue());
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_handle != InvalidHandleValue();
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return IsValid();
        }

    private:
        void Close() noexcept {
            if (IsValid() && m_deleter) {
                m_deleter(m_handle);
            }
        }

        NativeHandle m_handle;
        HandleDeleterFunc m_deleter;
    };

}
