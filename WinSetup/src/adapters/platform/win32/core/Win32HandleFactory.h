// src/adapters/platform/win32/core/Win32HandleFactory.h
#pragma once

#include <domain/memory/UniqueHandle.h>
#include <Windows.h>

namespace winsetup::adapters::platform {

    class Win32HandleFactory {
    public:
        static void CloseHandleDeleter(domain::NativeHandle handle) noexcept {
            if (handle && handle != domain::InvalidHandleValue()) {
                ::CloseHandle(static_cast<HANDLE>(handle));
            }
        }

        static void FreeLibraryDeleter(domain::NativeHandle handle) noexcept {
            if (handle) {
                ::FreeLibrary(static_cast<HMODULE>(handle));
            }
        }

        static void FindCloseDeleter(domain::NativeHandle handle) noexcept {
            if (handle && handle != domain::InvalidHandleValue()) {
                ::FindClose(static_cast<HANDLE>(handle));
            }
        }

        [[nodiscard]] static domain::UniqueHandle MakeHandle(HANDLE h) noexcept {
            return domain::UniqueHandle(
                reinterpret_cast<domain::NativeHandle>(h),
                CloseHandleDeleter
            );
        }

        [[nodiscard]] static domain::UniqueLibrary MakeLibrary(HMODULE h) noexcept {
            return domain::UniqueLibrary(
                reinterpret_cast<domain::NativeHandle>(h),
                FreeLibraryDeleter
            );
        }

        [[nodiscard]] static domain::UniqueFindHandle MakeFindHandle(HANDLE h) noexcept {
            return domain::UniqueFindHandle(
                reinterpret_cast<domain::NativeHandle>(h),
                FindCloseDeleter
            );
        }

        [[nodiscard]] static HANDLE ToWin32Handle(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HANDLE>(handle.Get());
        }

        [[nodiscard]] static HMODULE ToWin32Module(const domain::UniqueLibrary& lib) noexcept {
            return static_cast<HMODULE>(lib.Get());
        }

        [[nodiscard]] static HANDLE ToWin32FindHandle(const domain::UniqueFindHandle& handle) noexcept {
            return static_cast<HANDLE>(handle.Get());
        }
    };

}
