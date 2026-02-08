// src/domain/memory/UniqueLibrary.h

#pragma once

#include <domain/memory/UniqueHandle.h>
#include <utility>

namespace winsetup::domain {

    class UniqueLibrary {
    public:
        explicit UniqueLibrary(NativeHandle hModule = nullptr) noexcept
            : m_hModule(hModule)
        {
        }

        ~UniqueLibrary() noexcept = default;

        UniqueLibrary(const UniqueLibrary&) = delete;
        UniqueLibrary& operator=(const UniqueLibrary&) = delete;

        UniqueLibrary(UniqueLibrary&& other) noexcept
            : m_hModule(std::exchange(other.m_hModule, nullptr))
        {
        }

        UniqueLibrary& operator=(UniqueLibrary&& other) noexcept {
            if (this != &other) {
                m_hModule = std::exchange(other.m_hModule, nullptr);
            }
            return *this;
        }

        [[nodiscard]] NativeHandle Get() const noexcept { return m_hModule; }
        [[nodiscard]] explicit operator bool() const noexcept { return m_hModule != nullptr; }

    private:
        NativeHandle m_hModule;
    };

}
