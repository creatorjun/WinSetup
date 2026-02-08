// src/domain/memory/UniqueHandle.h

#pragma once

#include <Windows.h>
#include <utility>

namespace winsetup::domain {

    class UniqueHandle {
    public:
        explicit UniqueHandle(HANDLE handle = INVALID_HANDLE_VALUE) noexcept
            : m_handle(handle)
        {
        }

        ~UniqueHandle() noexcept {
            Close();
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : m_handle(std::exchange(other.m_handle, INVALID_HANDLE_VALUE))
        {
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Close();
                m_handle = std::exchange(other.m_handle, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return m_handle;
        }

        [[nodiscard]] HANDLE Release() noexcept {
            return std::exchange(m_handle, INVALID_HANDLE_VALUE);
        }

        void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept {
            Close();
            m_handle = handle;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr;
        }

        [[nodiscard]] HANDLE* AddressOf() noexcept {
            return &m_handle;
        }

    private:
        void Close() noexcept {
            if (m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr) {
                CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
        }

        HANDLE m_handle;
    };

    class UniqueLibrary {
    public:
        explicit UniqueLibrary(HMODULE hModule = nullptr) noexcept
            : m_hModule(hModule)
        {
        }

        ~UniqueLibrary() noexcept {
            if (m_hModule) {
                FreeLibrary(m_hModule);
            }
        }

        UniqueLibrary(const UniqueLibrary&) = delete;
        UniqueLibrary& operator=(const UniqueLibrary&) = delete;

        UniqueLibrary(UniqueLibrary&& other) noexcept
            : m_hModule(std::exchange(other.m_hModule, nullptr))
        {
        }

        UniqueLibrary& operator=(UniqueLibrary&& other) noexcept {
            if (this != &other) {
                if (m_hModule) {
                    FreeLibrary(m_hModule);
                }
                m_hModule = std::exchange(other.m_hModule, nullptr);
            }
            return *this;
        }

        [[nodiscard]] HMODULE Get() const noexcept {
            return m_hModule;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_hModule != nullptr;
        }

    private:
        HMODULE m_hModule;
    };

    class UniqueFindHandle {
    public:
        explicit UniqueFindHandle(HANDLE hFind = INVALID_HANDLE_VALUE) noexcept
            : m_hFind(hFind)
        {
        }

        ~UniqueFindHandle() noexcept {
            if (m_hFind != INVALID_HANDLE_VALUE) {
                FindClose(m_hFind);
            }
        }

        UniqueFindHandle(const UniqueFindHandle&) = delete;
        UniqueFindHandle& operator=(const UniqueFindHandle&) = delete;

        UniqueFindHandle(UniqueFindHandle&& other) noexcept
            : m_hFind(std::exchange(other.m_hFind, INVALID_HANDLE_VALUE))
        {
        }

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
            if (this != &other) {
                if (m_hFind != INVALID_HANDLE_VALUE) {
                    FindClose(m_hFind);
                }
                m_hFind = std::exchange(other.m_hFind, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return m_hFind;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_hFind != INVALID_HANDLE_VALUE;
        }

    private:
        HANDLE m_hFind;
    };

}
