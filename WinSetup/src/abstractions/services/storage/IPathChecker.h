// src/abstractions/services/storage/IPathChecker.h
#pragma once
#include <string>

namespace winsetup::abstractions {

    class IPathChecker {
    public:
        virtual ~IPathChecker() = default;

        [[nodiscard]] virtual bool Exists(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) const noexcept = 0;

        [[nodiscard]] virtual bool IsFile(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) const noexcept = 0;

        [[nodiscard]] virtual bool IsDirectory(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) const noexcept = 0;
    };

}
