// src/adapters/persistence/filesystem/Win32PathChecker.cpp
#include <adapters/persistence/filesystem/Win32PathChecker.h>
#include <Windows.h>
#include <winioctl.h>

namespace winsetup::adapters::persistence {

    namespace {

        std::wstring BuildFullPath(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) noexcept {
            std::wstring base = volumeGuid;
            if (!base.empty() && base.back() != L'\\')
                base += L'\\';

            std::wstring rel = relativePath;
            if (!rel.empty() && (rel.front() == L'\\' || rel.front() == L'/'))
                rel = rel.substr(1);

            for (auto& c : rel)
                if (c == L'/') c = L'\\';

            return base + rel;
        }

        DWORD GetAttributes(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) noexcept {
            return ::GetFileAttributesW(BuildFullPath(volumeGuid, relativePath).c_str());
        }

        std::optional<uint32_t> QueryDiskIndexByHandle(HANDLE hVol) noexcept {
            STORAGE_DEVICE_NUMBER sdn{};
            DWORD bytesReturned = 0;
            const BOOL ok = ::DeviceIoControl(
                hVol,
                IOCTL_STORAGE_GET_DEVICE_NUMBER,
                nullptr, 0,
                &sdn, sizeof(sdn),
                &bytesReturned, nullptr
            );
            if (!ok) return std::nullopt;
            return static_cast<uint32_t>(sdn.DeviceNumber);
        }

    }

    bool Win32PathChecker::Exists(
        const std::wstring& volumeGuid,
        const std::wstring& relativePath
    ) const noexcept {
        return GetAttributes(volumeGuid, relativePath) != INVALID_FILE_ATTRIBUTES;
    }

    bool Win32PathChecker::IsFile(
        const std::wstring& volumeGuid,
        const std::wstring& relativePath
    ) const noexcept {
        const DWORD attr = GetAttributes(volumeGuid, relativePath);
        if (attr == INVALID_FILE_ATTRIBUTES) return false;
        return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    bool Win32PathChecker::IsDirectory(
        const std::wstring& volumeGuid,
        const std::wstring& relativePath
    ) const noexcept {
        const DWORD attr = GetAttributes(volumeGuid, relativePath);
        if (attr == INVALID_FILE_ATTRIBUTES) return false;
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    std::optional<uint32_t> Win32PathChecker::FindDiskIndexByVolumeGuid(
        const std::wstring& volumeGuid
    ) const noexcept {
        if (volumeGuid.empty()) return std::nullopt;

        wchar_t mountBuf[MAX_PATH] = {};
        DWORD   mountBufLen = MAX_PATH;
        if (::GetVolumePathNamesForVolumeNameW(
            volumeGuid.c_str(), mountBuf, mountBufLen, &mountBufLen)
            || ::GetLastError() == ERROR_MORE_DATA)
        {
            const std::wstring mountPath(mountBuf);
            if (mountPath.size() >= 2) {
                const std::wstring query = L"\\\\.\\" + mountPath.substr(0, 2);
                HANDLE hVol = ::CreateFileW(
                    query.c_str(), 0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, 0, nullptr
                );
                if (hVol != INVALID_HANDLE_VALUE) {
                    auto result = QueryDiskIndexByHandle(hVol);
                    ::CloseHandle(hVol);
                    if (result.has_value()) return result;
                }
            }
        }
        std::wstring guidPath = volumeGuid;
        if (!guidPath.empty() && guidPath.back() == L'\\')
            guidPath.pop_back();

        HANDLE hVol = ::CreateFileW(
            guidPath.c_str(), 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr
        );
        if (hVol == INVALID_HANDLE_VALUE) return std::nullopt;

        auto result = QueryDiskIndexByHandle(hVol);
        ::CloseHandle(hVol);
        return result;
    }

}
