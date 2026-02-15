// src/adapters/platform/win32/core/UniqueHandle.h
#pragma once

#include <utility>
#include <cstdint>

namespace winsetup::adapters::platform {

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

}
