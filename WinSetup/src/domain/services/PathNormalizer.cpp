// src/domain/services/PathNormalizer.cpp
#include "PathNormalizer.h"
#include <algorithm>

namespace winsetup::domain {

    Expected<std::wstring> PathNormalizer::NormalizePath(const std::wstring& path) {
        if (path.empty()) {
            return Error{ L"Path is empty", 0, ErrorCategory::Unknown };
        }

        std::wstring normalized = path;
        std::replace(normalized.begin(), normalized.end(), L'/', L'\\');

        while (normalized.find(L"\\\\") != std::wstring::npos) {
            size_t pos = normalized.find(L"\\\\");
            normalized.erase(pos, 1);
        }

        return normalized;
    }

    Expected<std::wstring> PathNormalizer::ToAbsolutePath(const std::wstring& path) {
        auto normalized = NormalizePath(path);
        if (!normalized.HasValue()) {
            return normalized;
        }

        std::wstring result = normalized.Value();

        if (result.length() >= 2 && result[1] == L':') {
            return result;
        }

        return Error{ L"Cannot convert to absolute path", 0, ErrorCategory::Unknown };
    }

    bool PathNormalizer::IsValidWindowsPath(const std::wstring& path) noexcept {
        if (path.empty()) return false;
        if (path.length() < 3) return false;
        if (path[1] != L':') return false;
        if (path[2] != L'\\') return false;

        wchar_t drive = path[0];
        return (drive >= L'A' && drive <= L'Z') || (drive >= L'a' && drive <= L'z');
    }

    std::wstring PathNormalizer::RemoveTrailingSlash(std::wstring path) {
        if (!path.empty() && path.back() == L'\\') {
            path.pop_back();
        }
        return path;
    }

    std::wstring PathNormalizer::AddTrailingSlash(std::wstring path) {
        if (!path.empty() && path.back() != L'\\') {
            path += L'\\';
        }
        return path;
    }

}
