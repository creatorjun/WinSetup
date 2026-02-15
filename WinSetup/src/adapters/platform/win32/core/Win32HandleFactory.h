// src/adapters/platform/win32/core/Win32HandleFactory.h
#pragma once

#include <domain/memory/UniqueHandle.h>
#include <domain/memory/UniqueLibrary.h>
#include <domain/memory/UniqueFindHandle.h>
#include <Windows.h>

namespace winsetup::adapters::platform {

    class Win32HandleFactory {
    public:
        Win32HandleFactory() = delete;

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

        static void FindVolumeCloseDeleter(domain::NativeHandle handle) noexcept {
            if (handle && handle != domain::InvalidHandleValue()) {
                ::FindVolumeClose(static_cast<HANDLE>(handle));
            }
        }

        static void DeleteObjectDeleter(domain::NativeHandle handle) noexcept {
            if (handle) {
                ::DeleteObject(static_cast<HGDIOBJ>(handle));
            }
        }

        static void DeleteDCDeleter(domain::NativeHandle handle) noexcept {
            if (handle) {
                ::DeleteDC(static_cast<HDC>(handle));
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

        [[nodiscard]] static domain::UniqueFindHandle MakeFindVolumeHandle(HANDLE h) noexcept {
            return domain::UniqueFindHandle(
                reinterpret_cast<domain::NativeHandle>(h),
                FindVolumeCloseDeleter
            );
        }

        [[nodiscard]] static domain::UniqueHandle MakeGdiObject(HGDIOBJ h) noexcept {
            return domain::UniqueHandle(
                reinterpret_cast<domain::NativeHandle>(h),
                DeleteObjectDeleter
            );
        }

        [[nodiscard]] static domain::UniqueHandle MakeDC(HDC h) noexcept {
            return domain::UniqueHandle(
                reinterpret_cast<domain::NativeHandle>(h),
                DeleteDCDeleter
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

        [[nodiscard]] static HGDIOBJ ToWin32GdiObject(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HGDIOBJ>(handle.Get());
        }

        [[nodiscard]] static HFONT ToWin32Font(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HFONT>(handle.Get());
        }

        [[nodiscard]] static HBRUSH ToWin32Brush(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HBRUSH>(handle.Get());
        }

        [[nodiscard]] static HPEN ToWin32Pen(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HPEN>(handle.Get());
        }

        [[nodiscard]] static HBITMAP ToWin32Bitmap(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HBITMAP>(handle.Get());
        }

        [[nodiscard]] static HDC ToWin32DC(const domain::UniqueHandle& handle) noexcept {
            return static_cast<HDC>(handle.Get());
        }
    };

}
