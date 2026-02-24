// src/adapters/persistence/filesystem/Win32PathChecker.cpp
#include <adapters/persistence/filesystem/Win32PathChecker.h>
#include <Windows.h>

namespace winsetup::adapters::persistence {

    std::wstring Win32PathChecker::BuildFullPath(
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

    DWORD Win32PathChecker::GetAttributes(
        const std::wstring& volumeGuid,
        const std::wstring& relativePath
    ) noexcept {
        const std::wstring fullPath = BuildFullPath(volumeGuid, relativePath);
        return ::GetFileAttributesW(fullPath.c_str());
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

}
