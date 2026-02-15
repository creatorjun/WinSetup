// src/adapters/platform/win32/core/UniqueLibrary.h
#pragma once

#include "UniqueHandle.h"

namespace winsetup::adapters::platform {

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

}
