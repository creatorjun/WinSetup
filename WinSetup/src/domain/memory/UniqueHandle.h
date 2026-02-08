// src/domain/memory/UniqueHandle.h
#pragma once

#include <utility>
#include <cstdint>
#include <functional>

namespace winsetup::domain {

    using NativeHandle = void*;

    inline constexpr NativeHandle InvalidHandleValue() noexcept {
        return reinterpret_cast<NativeHandle>(static_cast<intptr_t>(-1));
    }

    using HandleDeleter = std::function<void(NativeHandle)>;

    class UniqueHandle {
    public:
        UniqueHandle() noexcept
            : m_handle(InvalidHandleValue())
        {
        }

        explicit UniqueHandle(NativeHandle handle, HandleDeleter deleter) noexcept
            : m_handle(handle)
            , m_deleter(std::move(deleter))
        {
        }

        ~UniqueHandle() noexcept {
            Close();
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : m_handle(std::exchange(other.m_handle, InvalidHandleValue()))
            , m_deleter(std::move(other.m_deleter))
        {
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, InvalidHandleValue());
                m_deleter = std::move(other.m_deleter);
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

        void Reset(NativeHandle handle = InvalidHandleValue(), HandleDeleter deleter = nullptr) noexcept {
            Close();
            m_handle = handle;
            m_deleter = std::move(deleter);
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
                m_handle = InvalidHandleValue();
            }
        }

        NativeHandle m_handle;
        HandleDeleter m_deleter;
    };

    class UniqueLibrary {
    public:
        UniqueLibrary() noexcept
            : m_handle(nullptr)
        {
        }

        explicit UniqueLibrary(NativeHandle handle, HandleDeleter deleter) noexcept
            : m_handle(handle)
            , m_deleter(std::move(deleter))
        {
        }

        ~UniqueLibrary() noexcept {
            Close();
        }

        UniqueLibrary(const UniqueLibrary&) = delete;
        UniqueLibrary& operator=(const UniqueLibrary&) = delete;

        UniqueLibrary(UniqueLibrary&& other) noexcept
            : m_handle(std::exchange(other.m_handle, nullptr))
            , m_deleter(std::move(other.m_deleter))
        {
        }

        UniqueLibrary& operator=(UniqueLibrary&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, nullptr);
                m_deleter = std::move(other.m_deleter);
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
                m_handle = nullptr;
            }
        }

        NativeHandle m_handle;
        HandleDeleter m_deleter;
    };

    class UniqueFindHandle {
    public:
        UniqueFindHandle() noexcept
            : m_handle(InvalidHandleValue())
        {
        }

        explicit UniqueFindHandle(NativeHandle handle, HandleDeleter deleter) noexcept
            : m_handle(handle)
            , m_deleter(std::move(deleter))
        {
        }

        ~UniqueFindHandle() noexcept {
            Close();
        }

        UniqueFindHandle(const UniqueFindHandle&) = delete;
        UniqueFindHandle& operator=(const UniqueFindHandle&) = delete;

        UniqueFindHandle(UniqueFindHandle&& other) noexcept
            : m_handle(std::exchange(other.m_handle, InvalidHandleValue()))
            , m_deleter(std::move(other.m_deleter))
        {
        }

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, InvalidHandleValue());
                m_deleter = std::move(other.m_deleter);
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
                m_handle = InvalidHandleValue();
            }
        }

        NativeHandle m_handle;
        HandleDeleter m_deleter;
    };

}
