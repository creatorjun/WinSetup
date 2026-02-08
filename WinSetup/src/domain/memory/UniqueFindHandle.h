// src/domain/memory/UniqueFindHandle.h

#pragma once

#include <domain/memory/UniqueHandle.h>
#include <utility>

namespace winsetup::domain {

    class UniqueFindHandle {
    public:
        explicit UniqueFindHandle(NativeHandle hFind = nullptr) noexcept
            : m_hFind(hFind)
        {
        }

        ~UniqueFindHandle() noexcept = default;

        UniqueFindHandle(const UniqueFindHandle&) = delete;
        UniqueFindHandle& operator=(const UniqueFindHandle&) = delete;

        UniqueFindHandle(UniqueFindHandle&& other) noexcept
            : m_hFind(std::exchange(other.m_hFind, nullptr))
        {
        }

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
            if (this != &other) {
                m_hFind = std::exchange(other.m_hFind, nullptr);
            }
            return *this;
        }

        [[nodiscard]] NativeHandle Get() const noexcept { return m_hFind; }
        [[nodiscard]] explicit operator bool() const noexcept {
            NativeHandle invalid = GetInvalidHandleValue();
            return m_hFind != nullptr && m_hFind != invalid;
        }

    private:
        NativeHandle m_hFind;
    };

}
