// src/domain/services/PathNormalizer.h
#pragma once

#include <string>
#include "../primitives/Expected.h"

namespace winsetup::domain {

    class PathNormalizer {
    public:
        [[nodiscard]] static Expected<std::wstring> NormalizePath(const std::wstring& path);
        [[nodiscard]] static Expected<std::wstring> ToAbsolutePath(const std::wstring& path);
        [[nodiscard]] static bool IsValidWindowsPath(const std::wstring& path) noexcept;
        [[nodiscard]] static std::wstring RemoveTrailingSlash(std::wstring path);
        [[nodiscard]] static std::wstring AddTrailingSlash(std::wstring path);
    };

}
