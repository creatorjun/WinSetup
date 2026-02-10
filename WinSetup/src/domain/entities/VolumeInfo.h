// src/domain/entities/VolumeInfo.h
#pragma once

#include <string>
#include <optional>
#include "../valueobjects/DiskSize.h"
#include "../valueobjects/FileSystemType.h"

namespace winsetup::domain {

    class VolumeInfo {
    public:
        VolumeInfo() = default;

        VolumeInfo(
            int index,
            const std::wstring& letter,
            const std::wstring& label,
            FileSystemType fileSystem,
            DiskSize size
        )
            : mindex(index)
            , mletter(letter)
            , mlabel(label)
            , mfileSystem(fileSystem)
            , msize(size)
        {
        }

        [[nodiscard]] int GetIndex() const noexcept { return mindex; }
        [[nodiscard]] const std::wstring& GetLetter() const noexcept { return mletter; }
        [[nodiscard]] const std::wstring& GetLabel() const noexcept { return mlabel; }
        [[nodiscard]] FileSystemType GetFileSystem() const noexcept { return mfileSystem; }
        [[nodiscard]] DiskSize GetSize() const noexcept { return msize; }
        [[nodiscard]] const std::wstring& GetVolumeType() const noexcept { return mvolumeType; }
        [[nodiscard]] const std::wstring& GetVolumePath() const noexcept { return mvolumePath; }
        [[nodiscard]] bool IsMounted() const noexcept { return misMounted; }
        [[nodiscard]] bool IsReadOnly() const noexcept { return misReadOnly; }

        void SetIndex(int index) noexcept { mindex = index; }
        void SetLetter(const std::wstring& letter) { mletter = letter; }
        void SetLetter(std::wstring&& letter) noexcept { mletter = std::move(letter); }
        void SetLabel(const std::wstring& label) { mlabel = label; }
        void SetLabel(std::wstring&& label) noexcept { mlabel = std::move(label); }
        void SetFileSystem(FileSystemType fileSystem) noexcept { mfileSystem = fileSystem; }
        void SetSize(DiskSize size) noexcept { msize = size; }
        void SetVolumeType(const std::wstring& volumeType) { mvolumeType = volumeType; }
        void SetVolumeType(std::wstring&& volumeType) noexcept { mvolumeType = std::move(volumeType); }
        void SetVolumePath(const std::wstring& path) { mvolumePath = path; }
        void SetVolumePath(std::wstring&& path) noexcept { mvolumePath = std::move(path); }
        void SetMounted(bool mounted) noexcept { misMounted = mounted; }
        void SetReadOnly(bool readOnly) noexcept { misReadOnly = readOnly; }

        [[nodiscard]] bool IsValid() const noexcept {
            return mindex >= 0 && msize.ToBytes() > 0;
        }

        [[nodiscard]] bool IsSystemVolume() const noexcept {
            if (mvolumeType.empty()) {
                return false;
            }
            for (wchar_t c : mvolumeType) {
                wchar_t lower = static_cast<wchar_t>(towlower(c));
                if (lower != L's' && lower != L'y' && lower != L't' &&
                    lower != L'e' && lower != L'm') {
                    continue;
                }
                return mvolumeType.find(L"system") != std::wstring::npos ||
                    mvolumeType.find(L"System") != std::wstring::npos ||
                    mvolumeType.find(L"SYSTEM") != std::wstring::npos;
            }
            return false;
        }

        [[nodiscard]] bool IsBootVolume() const noexcept {
            if (mvolumeType.empty()) {
                return false;
            }
            return mvolumeType.find(L"boot") != std::wstring::npos ||
                mvolumeType.find(L"Boot") != std::wstring::npos ||
                mvolumeType.find(L"BOOT") != std::wstring::npos;
        }

        [[nodiscard]] bool HasEnoughSpace(DiskSize required) const noexcept {
            return msize >= required;
        }

        [[nodiscard]] double GetSizeGB() const noexcept {
            return msize.ToGB();
        }

        [[nodiscard]] bool IsNTFS() const noexcept {
            return mfileSystem == FileSystemType::NTFS;
        }

        [[nodiscard]] bool IsFAT32() const noexcept {
            return mfileSystem == FileSystemType::FAT32;
        }

        [[nodiscard]] bool CanInstallWindows() const noexcept {
            return IsNTFS() && HasEnoughSpace(DiskSize::FromGB(20));
        }

        [[nodiscard]] std::wstring GetDisplayName() const {
            if (!mlabel.empty()) {
                return mletter + L" (" + mlabel + L")";
            }
            return mletter;
        }

        static constexpr int INVALID_INDEX = -1;

    private:
        int mindex = INVALID_INDEX;
        std::wstring mletter;
        std::wstring mlabel;
        FileSystemType mfileSystem = FileSystemType::Unknown;
        DiskSize msize;
        std::wstring mvolumeType;
        std::wstring mvolumePath;
        bool misMounted = false;
        bool misReadOnly = false;
    };

}
