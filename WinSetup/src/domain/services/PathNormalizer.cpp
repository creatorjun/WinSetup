// src/domain/services/PathNormalizer.cpp

#include <domain/services/PathNormalizer.h>
#include <algorithm>

namespace winsetup::domain {

    std::wstring PathNormalizer::Normalize(const std::wstring& path) {
        if (path.empty()) {
            return path;
        }

        std::wstring normalized = path;
        std::replace(normalized.begin(), normalized.end(), ALT_SEPARATOR, SEPARATOR);

        size_t pos = 0;
        while ((pos = normalized.find(L"\\\\", pos)) != std::wstring::npos) {
            normalized.erase(pos, 1);
        }

        return RemoveTrailingSlash(normalized);
    }

    std::wstring PathNormalizer::RemoveTrailingSlash(const std::wstring& path) {
        if (path.empty() || path.length() <= 3) {
            return path;
        }

        if (path.back() == SEPARATOR || path.back() == ALT_SEPARATOR) {
            return path.substr(0, path.length() - 1);
        }

        return path;
    }

    std::wstring PathNormalizer::AddTrailingSlash(const std::wstring& path) {
        if (path.empty()) {
            return path;
        }

        if (path.back() != SEPARATOR && path.back() != ALT_SEPARATOR) {
            return path + SEPARATOR;
        }

        return path;
    }

    std::wstring PathNormalizer::GetFileName(const std::wstring& path) {
        auto normalized = Normalize(path);
        auto pos = normalized.find_last_of(SEPARATOR);

        if (pos == std::wstring::npos) {
            return normalized;
        }

        return normalized.substr(pos + 1);
    }

    std::wstring PathNormalizer::GetDirectory(const std::wstring& path) {
        auto normalized = Normalize(path);
        auto pos = normalized.find_last_of(SEPARATOR);

        if (pos == std::wstring::npos) {
            return L"";
        }

        return normalized.substr(0, pos);
    }

    std::wstring PathNormalizer::GetExtension(const std::wstring& path) {
        auto fileName = GetFileName(path);
        auto pos = fileName.find_last_of(L'.');

        if (pos == std::wstring::npos || pos == 0) {
            return L"";
        }

        return fileName.substr(pos);
    }

    std::wstring PathNormalizer::Combine(const std::wstring& path1, const std::wstring& path2) {
        if (path1.empty()) {
            return Normalize(path2);
        }

        if (path2.empty()) {
            return Normalize(path1);
        }

        auto normalized1 = RemoveTrailingSlash(Normalize(path1));
        auto normalized2 = path2;

        if (normalized2.front() == SEPARATOR || normalized2.front() == ALT_SEPARATOR) {
            normalized2 = normalized2.substr(1);
        }

        return normalized1 + SEPARATOR + normalized2;
    }

    std::vector<std::wstring> PathNormalizer::Split(const std::wstring& path) {
        std::vector<std::wstring> parts;
        auto normalized = Normalize(path);

        size_t start = 0;
        size_t pos = 0;

        while ((pos = normalized.find(SEPARATOR, start)) != std::wstring::npos) {
            if (pos > start) {
                parts.push_back(normalized.substr(start, pos - start));
            }
            start = pos + 1;
        }

        if (start < normalized.length()) {
            parts.push_back(normalized.substr(start));
        }

        return parts;
    }

    bool PathNormalizer::IsAbsolute(const std::wstring& path) noexcept {
        if (path.length() < 3) {
            return false;
        }

        if ((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')) {
            return path[1] == L':' && (path[2] == SEPARATOR || path[2] == ALT_SEPARATOR);
        }

        return (path[0] == SEPARATOR || path[0] == ALT_SEPARATOR) &&
            (path[1] == SEPARATOR || path[1] == ALT_SEPARATOR);
    }

    bool PathNormalizer::IsRelative(const std::wstring& path) noexcept {
        return !IsAbsolute(path);
    }

}
