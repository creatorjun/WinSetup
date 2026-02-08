// src/domain/memory/UniqueHandle.cpp

#include <domain/memory/UniqueHandle.h>

namespace winsetup::domain {

    UniqueHandle::UniqueHandle(NativeHandle handle) noexcept
        : m_handle(handle)
    {
    }

    UniqueHandle::~UniqueHandle() noexcept {
        Close();
    }

    UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr))
    {
    }

    UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            Close();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    NativeHandle UniqueHandle::Release() noexcept {
        return std::exchange(m_handle, nullptr);
    }

    void UniqueHandle::Reset(NativeHandle handle) noexcept {
        Close();
        m_handle = handle;
    }

    UniqueHandle::operator bool() const noexcept {
        return IsValid();
    }

    bool UniqueHandle::IsValid() const noexcept {
        NativeHandle invalid = GetInvalidHandleValue();
        return m_handle != nullptr && m_handle != invalid;
    }

    void UniqueHandle::Close() noexcept {
    }

}
