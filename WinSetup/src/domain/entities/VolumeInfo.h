#pragma once

#include <string>
#include <optional>
#include "../primitives/Expected.h"

namespace winsetup::domain {

    enum class FileSystemType : uint8_t {
        Unknown,
        NTFS,
        FAT32,
        exFAT,
        ReFS
    };

    [[nodiscard]] constexpr const wchar_t* FileSystemTypeToString(FileSystemType type) noexcept {
        switch (type) {
        case FileSystemType::NTFS:   return L"NTFS";
        case FileSystemType::FAT32:  return L"FAT32";
        case FileSystemType::exFAT:  return L"exFAT";
        case FileSystemType::ReFS:   return L"ReFS";
        default:                     return L"Unknown";
        }
    }

    enum class VolumeRole : uint8_t {
        Unknown,
        System,
        Boot,
        Data,
        Recovery
    };

    class VolumeInfo {
    public:
        VolumeInfo() = default;

        VolumeInfo(
            std::wstring volumePath,
            std::optional<wchar_t> driveLetter,
            FileSystemType fsType,
            uint64_t totalSize,
            uint64_t freeSize
        )
            : volumePath_(std::move(volumePath))
            , driveLetter_(driveLetter)
            , fileSystemType_(fsType)
            , totalSizeInBytes_(totalSize)
            , freeSizeInBytes_(freeSize)
            , role_(VolumeRole::Unknown) {
        }

        [[nodiscard]] const std::wstring& GetVolumePath() const noexcept {
            return volumePath_;
        }

        [[nodiscard]] std::optional<wchar_t> GetDriveLetter() const noexcept {
            return driveLetter_;
        }

        [[nodiscard]] std::wstring GetDriveLetterString() const {
            if (driveLetter_.has_value()) {
                return std::wstring(1, driveLetter_.value()) + L":";
            }
            return L"";
        }

        [[nodiscard]] FileSystemType GetFileSystemType() const noexcept {
            return fileSystemType_;
        }

        [[nodiscard]] uint64_t GetTotalSizeInBytes() const noexcept {
            return totalSizeInBytes_;
        }

        [[nodiscard]] uint64_t GetFreeSizeInBytes() const noexcept {
            return freeSizeInBytes_;
        }

        [[nodiscard]] uint64_t GetUsedSizeInBytes() const noexcept {
            return totalSizeInBytes_ - freeSizeInBytes_;
        }

        [[nodiscard]] double GetUsagePercentage() const noexcept {
            if (totalSizeInBytes_ == 0) return 0.0;
            return static_cast<double>(GetUsedSizeInBytes()) /
                static_cast<double>(totalSizeInBytes_) * 100.0;
        }

        [[nodiscard]] VolumeRole GetRole() const noexcept {
            return role_;
        }

        void SetRole(VolumeRole role) noexcept {
            role_ = role;
        }

        [[nodiscard]] const std::wstring& GetLabel() const noexcept {
            return label_;
        }

        void SetLabel(std::wstring label) {
            label_ = std::move(label);
        }

        [[nodiscard]] bool HasDriveLetter() const noexcept {
            return driveLetter_.has_value();
        }

        [[nodiscard]] bool IsNTFS() const noexcept {
            return fileSystemType_ == FileSystemType::NTFS;
        }

        [[nodiscard]] bool HasEnoughSpace(uint64_t requiredBytes) const noexcept {
            return freeSizeInBytes_ >= requiredBytes;
        }

    private:
        std::wstring volumePath_;
        std::optional<wchar_t> driveLetter_;
        std::wstring label_;
        FileSystemType fileSystemType_{ FileSystemType::Unknown };
        uint64_t totalSizeInBytes_{ 0 };
        uint64_t freeSizeInBytes_{ 0 };
        VolumeRole role_{ VolumeRole::Unknown };
    };

}
