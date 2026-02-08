// src/domain/services/PathNormalizer.cpp
#include <domain/services/PathNormalizer.h>
#include <algorithm>

namespace winsetup::domain {

    std::wstring PathNormalizer::Normalize(const std::wstring& path) {
        if (path.empty()) {
            return path;
        }

        std::wstring normalized;
        normalized.reserve(path.length());

        wchar_t prev = L'\0';
        for (wchar_t c : path) {
            if (c == ALT_SEPARATOR) {
                c = SEPARATOR;
            }

            if (c == SEPARATOR && prev == SEPARATOR) {
                continue;
            }

            normalized += c;
            prev = c;
        }

        if (normalized.length() > 3 && normalized.back() == SEPARATOR) {
            normalized.pop_back();
        }

        return normalized;
    }

    std::wstring PathNormalizer::Combine(const std::wstring& path1, const std::wstring& path2) {
        if (path1.empty()) return path2;
        if (path2.empty()) return path1;

        std::wstring result;
        result.reserve(path1.length() + path2.length() + 1);

        result = path1;
        if (result.back() != SEPARATOR && result.back() != ALT_SEPARATOR) {
            result += SEPARATOR;
        }

        size_t start = (path2.front() == SEPARATOR || path2.front() == ALT_SEPARATOR) ? 1 : 0;
        result.append(path2, start, std::wstring::npos);

        return Normalize(result);
    }

    std::wstring PathNormalizer::GetDirectory(const std::wstring& path) {
        auto pos = path.find_last_of(SEPARATOR);
        if (pos == std::wstring::npos) {
            return L"";
        }
        return path.substr(0, pos);
    }

    std::wstring PathNormalizer::GetFileName(const std::wstring& path) {
        auto pos = path.find_last_of(SEPARATOR);
        if (pos == std::wstring::npos) {
            return path;
        }
        return path.substr(pos + 1);
    }

    std::wstring PathNormalizer::GetExtension(const std::wstring& path) {
        auto fileName = GetFileName(path);
        auto pos = fileName.find_last_of(L'.');
        if (pos == std::wstring::npos || pos == 0) {
            return L"";
        }
        return fileName.substr(pos);
    }

    bool PathNormalizer::IsAbsolute(const std::wstring& path) noexcept {
        if (path.length() < 2) {
            return false;
        }
        return (path[1] == L':') || (path[0] == SEPARATOR && path[1] == SEPARATOR);
    }

    bool PathNormalizer::HasTrailingSeparator(const std::wstring& path) noexcept {
        if (path.empty()) {
            return false;
        }
        wchar_t last = path.back();
        return last == SEPARATOR || last == ALT_SEPARATOR;
    }

}
