// src/abstractions/services/storage/IPathChecker.h
#pragma once
#include <string>
#include <optional>
#include <cstdint>

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

        [[nodiscard]] virtual std::optional<uint32_t> FindDiskIndexByVolumeGuid(
            const std::wstring& volumeGuid
        ) const noexcept = 0;
    };

}
