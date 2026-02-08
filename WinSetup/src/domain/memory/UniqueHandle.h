// src/domain/memory/UniqueHandle.h
#pragma once

#include <windows.h>
#include <utility>

namespace winsetup::domain {

    class UniqueHandle {
    public:
        constexpr UniqueHandle() noexcept : m_handle(INVALID_HANDLE_VALUE) {}

        explicit UniqueHandle(HANDLE handle) noexcept : m_handle(handle) {}

        ~UniqueHandle() noexcept {
            Close();
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : m_handle(other.m_handle)
        {
            other.m_handle = INVALID_HANDLE_VALUE;
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = other.m_handle;
                other.m_handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept { return m_handle; }
        [[nodiscard]] HANDLE* GetAddressOf() noexcept { return &m_handle; }
        [[nodiscard]] bool IsValid() const noexcept {
            return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return IsValid();
        }

        [[nodiscard]] HANDLE Release() noexcept {
            HANDLE h = m_handle;
            m_handle = INVALID_HANDLE_VALUE;
            return h;
        }

        void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept {
            Close();
            m_handle = handle;
        }

        void Close() noexcept {
            if (IsValid()) {
                ::CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
        }

        void Attach(HANDLE handle) noexcept {
            Close();
            m_handle = handle;
        }

        [[nodiscard]] HANDLE Detach() noexcept {
            return Release();
        }

    private:
        HANDLE m_handle;
    };

}
