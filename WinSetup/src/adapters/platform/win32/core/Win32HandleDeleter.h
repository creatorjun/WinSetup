// src/adapters/platform/win32/core/Win32HandleDeleter.h
#pragma once

#include <domain/memory/UniqueHandle.h>
#include <Windows.h>

namespace winsetup::adapters::platform {

    inline void Win32HandleDeleter(domain::NativeHandle handle) noexcept {
        if (handle && handle != reinterpret_cast<domain::NativeHandle>(-1)) {
            ::CloseHandle(static_cast<HANDLE>(handle));
        }
    }

    inline void Win32LibraryDeleter(domain::NativeHandle handle) noexcept {
        if (handle) {
            ::FreeLibrary(static_cast<HMODULE>(handle));
        }
    }

    inline void Win32FindHandleDeleter(domain::NativeHandle handle) noexcept {
        if (handle && handle != reinterpret_cast<domain::NativeHandle>(-1)) {
            ::FindClose(static_cast<HANDLE>(handle));
        }
    }

    // Factory functions
    inline domain::UniqueHandle MakeUniqueHandle(HANDLE h) noexcept {
        return domain::UniqueHandle(
            reinterpret_cast<domain::NativeHandle>(h),
            Win32HandleDeleter
        );
    }

}
