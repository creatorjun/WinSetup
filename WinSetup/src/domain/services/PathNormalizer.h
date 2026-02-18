// src/domain/services/PathNormalizer.h
#pragma once

#include <string>

namespace winsetup::domain {

    class PathNormalizer {
    public:
        PathNormalizer() = default;
        ~PathNormalizer() = default;

        [[nodiscard]] std::wstring ResolvePath(
            const std::wstring& path,
            const std::wstring& userProfile
        ) const;

        [[nodiscard]] static std::wstring SanitizeFolderName(const std::wstring& name);
    };

}
