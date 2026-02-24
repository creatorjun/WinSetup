// src/adapters/persistence/filesystem/Win32PathChecker.h
#pragma once
#include <abstractions/services/storage/IPathChecker.h>
#include <string>

namespace winsetup::adapters::persistence {

    class Win32PathChecker final : public abstractions::IPathChecker {
    public:
        Win32PathChecker() = default;
        ~Win32PathChecker() override = default;
        Win32PathChecker(const Win32PathChecker&) = delete;
        Win32PathChecker& operator=(const Win32PathChecker&) = delete;

        [[nodiscard]] bool Exists(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) const noexcept override;

        [[nodiscard]] bool IsFile(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) const noexcept override;

        [[nodiscard]] bool IsDirectory(
            const std::wstring& volumeGuid,
            const std::wstring& relativePath
        ) const noexcept override;
    };

}
