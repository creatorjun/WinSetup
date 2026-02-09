// src/domain/memory/UniqueHandle.h
#pragma once

#include <utility>
#include <cstdint>

namespace winsetup::domain {

    using NativeHandle = void*;
    using HandleDeleterFunc = void(*)(NativeHandle) noexcept;

    inline constexpr NativeHandle InvalidHandleValue() noexcept {
        return reinterpret_cast<NativeHandle>(static_cast<intptr_t>(-1));
    }

    class UniqueHandle {
    public:
        explicit UniqueHandle(
            NativeHandle handle = nullptr,
            HandleDeleterFunc deleter = nullptr
        ) noexcept
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
            : m_handle(std::exchange(other.m_handle, nullptr))
            , m_deleter(std::exchange(other.m_deleter, nullptr))
        {
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
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

        void Reset(NativeHandle handle = nullptr, HandleDeleterFunc deleter = nullptr) noexcept {
            Close();
            m_handle = handle;
            m_deleter = deleter;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_handle != nullptr && m_handle != InvalidHandleValue();
        }

    private:
        void Close() noexcept {
            if (m_handle && m_handle != InvalidHandleValue() && m_deleter) {
                m_deleter(m_handle);
                m_handle = nullptr;
                m_deleter = nullptr;
            }
        }

        NativeHandle m_handle;
        HandleDeleterFunc m_deleter;
    };

    class UniqueLibrary {
    public:
        explicit UniqueLibrary(
            NativeHandle handle = nullptr,
            HandleDeleterFunc deleter = nullptr
        ) noexcept
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

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_handle != nullptr;
        }

    private:
        void Close() noexcept {
            if (m_handle && m_deleter) {
                m_deleter(m_handle);
                m_handle = nullptr;
                m_deleter = nullptr;
            }
        }

        NativeHandle m_handle;
        HandleDeleterFunc m_deleter;
    };

    class UniqueFindHandle {
    public:
        explicit UniqueFindHandle(
            NativeHandle handle = nullptr,
            HandleDeleterFunc deleter = nullptr
        ) noexcept
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
            : m_handle(std::exchange(other.m_handle, nullptr))
            , m_deleter(std::exchange(other.m_deleter, nullptr))
        {
        }

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
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

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_handle != nullptr && m_handle != InvalidHandleValue();
        }

    private:
        void Close() noexcept {
            if (m_handle && m_handle != InvalidHandleValue() && m_deleter) {
                m_deleter(m_handle);
                m_handle = nullptr;
                m_deleter = nullptr;
            }
        }

        NativeHandle m_handle;
        HandleDeleterFunc m_deleter;
    };

}
