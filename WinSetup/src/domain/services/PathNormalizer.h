// src/domain/services/PathNormalizer.h
#pragma once

#include <string>

namespace winsetup::domain {

    class PathNormalizer {
    public:
        static std::wstring Normalize(const std::wstring& path);
        static std::wstring Combine(const std::wstring& path1, const std::wstring& path2);
        static std::wstring GetDirectory(const std::wstring& path);
        static std::wstring GetFileName(const std::wstring& path);
        static std::wstring GetExtension(const std::wstring& path);
        static bool IsAbsolute(const std::wstring& path) noexcept;
        static bool HasTrailingSeparator(const std::wstring& path) noexcept;

        static constexpr wchar_t SEPARATOR = L'\\';
        static constexpr wchar_t ALT_SEPARATOR = L'/';
    };

}
