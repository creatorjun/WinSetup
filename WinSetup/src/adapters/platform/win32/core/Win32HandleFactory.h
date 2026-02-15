// src/adapters/platform/win32/core/Win32HandleFactory.h
#pragma once

#include "../memory/UniqueHandle.h"
#include "../memory/UniqueLibrary.h"
#include "../memory/UniqueFindHandle.h"
#include <Windows.h>

namespace winsetup::adapters::platform {

    class Win32HandleFactory {
    public:
        Win32HandleFactory() = delete;

        static void CloseHandleDeleter(NativeHandle handle) noexcept {
            if (handle && handle != InvalidHandleValue()) {
                ::CloseHandle(static_cast<HANDLE>(handle));
            }
        }

        static void FreeLibraryDeleter(NativeHandle handle) noexcept {
            if (handle) {
                ::FreeLibrary(static_cast<HMODULE>(handle));
            }
        }

        static void FindCloseDeleter(NativeHandle handle) noexcept {
            if (handle && handle != InvalidHandleValue()) {
                ::FindClose(static_cast<HANDLE>(handle));
            }
        }

        static void FindVolumeCloseDeleter(NativeHandle handle) noexcept {
            if (handle && handle != InvalidHandleValue()) {
                ::FindVolumeClose(static_cast<HANDLE>(handle));
            }
        }

        static void DeleteObjectDeleter(NativeHandle handle) noexcept {
            if (handle) {
                ::DeleteObject(static_cast<HGDIOBJ>(handle));
            }
        }

        static void DeleteDCDeleter(NativeHandle handle) noexcept {
            if (handle) {
                ::DeleteDC(static_cast<HDC>(handle));
            }
        }

        [[nodiscard]] static UniqueHandle MakeHandle(HANDLE h) noexcept {
            return UniqueHandle(
                reinterpret_cast<NativeHandle>(h),
                CloseHandleDeleter
            );
        }

        [[nodiscard]] static UniqueLibrary MakeLibrary(HMODULE h) noexcept {
            return UniqueLibrary(
                reinterpret_cast<NativeHandle>(h),
                FreeLibraryDeleter
            );
        }

        [[nodiscard]] static UniqueFindHandle MakeFindHandle(HANDLE h) noexcept {
            return UniqueFindHandle(
                reinterpret_cast<NativeHandle>(h),
                FindCloseDeleter
            );
        }

        [[nodiscard]] static UniqueFindHandle MakeFindVolumeHandle(HANDLE h) noexcept {
            return UniqueFindHandle(
                reinterpret_cast<NativeHandle>(h),
                FindVolumeCloseDeleter
            );
        }

        [[nodiscard]] static UniqueHandle MakeGdiObject(HGDIOBJ h) noexcept {
            return UniqueHandle(
                reinterpret_cast<NativeHandle>(h),
                DeleteObjectDeleter
            );
        }

        [[nodiscard]] static UniqueHandle MakeDC(HDC h) noexcept {
            return UniqueHandle(
                reinterpret_cast<NativeHandle>(h),
                DeleteDCDeleter
            );
        }

        [[nodiscard]] static HANDLE ToWin32Handle(const UniqueHandle& handle) noexcept {
            return static_cast<HANDLE>(handle.Get());
        }

        [[nodiscard]] static HMODULE ToWin32Module(const UniqueLibrary& lib) noexcept {
            return static_cast<HMODULE>(lib.Get());
        }

        [[nodiscard]] static HANDLE ToWin32FindHandle(const UniqueFindHandle& handle) noexcept {
            return static_cast<HANDLE>(handle.Get());
        }

        [[nodiscard]] static HGDIOBJ ToWin32GdiObject(const UniqueHandle& handle) noexcept {
            return static_cast<HGDIOBJ>(handle.Get());
        }

        [[nodiscard]] static HFONT ToWin32Font(const UniqueHandle& handle) noexcept {
            return static_cast<HFONT>(handle.Get());
        }

        [[nodiscard]] static HBRUSH ToWin32Brush(const UniqueHandle& handle) noexcept {
            return static_cast<HBRUSH>(handle.Get());
        }

        [[nodiscard]] static HPEN ToWin32Pen(const UniqueHandle& handle) noexcept {
            return static_cast<HPEN>(handle.Get());
        }

        [[nodiscard]] static HBITMAP ToWin32Bitmap(const UniqueHandle& handle) noexcept {
            return static_cast<HBITMAP>(handle.Get());
        }

        [[nodiscard]] static HDC ToWin32DC(const UniqueHandle& handle) noexcept {
            return static_cast<HDC>(handle.Get());
        }
    };

}
