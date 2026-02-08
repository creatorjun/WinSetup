// src/domain/memory/UniqueHandle.h

#pragma once

#include <utility>
#include <cstdint>

namespace winsetup::domain {

    using NativeHandle = void*;

    inline constexpr NativeHandle NULL_HANDLE = nullptr;

    inline NativeHandle GetInvalidHandleValue() noexcept {
        return reinterpret_cast<NativeHandle>(-1);
    }

    class UniqueHandle {
    public:
        explicit UniqueHandle(NativeHandle handle = nullptr) noexcept;
        ~UniqueHandle() noexcept;

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept;
        UniqueHandle& operator=(UniqueHandle&& other) noexcept;

        [[nodiscard]] NativeHandle Get() const noexcept { return m_handle; }
        [[nodiscard]] NativeHandle Release() noexcept;
        void Reset(NativeHandle handle = nullptr) noexcept;
        [[nodiscard]] explicit operator bool() const noexcept;
        [[nodiscard]] NativeHandle* AddressOf() noexcept { return &m_handle; }

        [[nodiscard]] bool IsValid() const noexcept;

    private:
        void Close() noexcept;
        NativeHandle m_handle;
    };

}
