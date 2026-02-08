// src/domain/entities/VolumeInfo.h
#pragma once

#include <string>
#include <string_view>
#include <domain/valueobjects/DriveLetter.h>
#include <domain/valueobjects/FileSystemType.h>
#include <domain/valueobjects/DiskSize.h>

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
            : m_index(index)
            , m_letter(letter)
            , m_label(label)
            , m_fileSystem(fileSystem)
            , m_size(size)
        {
        }

        [[nodiscard]] int GetIndex() const noexcept { return m_index; }
        [[nodiscard]] const std::wstring& GetLetter() const noexcept { return m_letter; }
        [[nodiscard]] const std::wstring& GetLabel() const noexcept { return m_label; }
        [[nodiscard]] FileSystemType GetFileSystem() const noexcept { return m_fileSystem; }
        [[nodiscard]] DiskSize GetSize() const noexcept { return m_size; }
        [[nodiscard]] const std::wstring& GetVolumeType() const noexcept { return m_volumeType; }

        void SetIndex(int index) noexcept { m_index = index; }
        void SetLetter(const std::wstring& letter) { m_letter = letter; }
        void SetLetter(std::wstring&& letter) noexcept { m_letter = std::move(letter); }
        void SetLabel(const std::wstring& label) { m_label = label; }
        void SetLabel(std::wstring&& label) noexcept { m_label = std::move(label); }
        void SetFileSystem(FileSystemType fileSystem) noexcept { m_fileSystem = fileSystem; }
        void SetSize(DiskSize size) noexcept { m_size = size; }
        void SetVolumeType(const std::wstring& volumeType) { m_volumeType = volumeType; }
        void SetVolumeType(std::wstring&& volumeType) noexcept { m_volumeType = std::move(volumeType); }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_index >= 0 && m_size.ToBytes() > 0;
        }

        [[nodiscard]] bool IsSystemVolume() const noexcept {
            if (m_volumeType.empty()) return false;

            for (wchar_t c : m_volumeType) {
                wchar_t lower = static_cast<wchar_t>(::towlower(c));
                if (lower == L's' || lower == L'y' || lower == L't' || lower == L'e' || lower == L'm') {
                    continue;
                }
            }
            return m_volumeType.find(L"system") != std::wstring::npos ||
                m_volumeType.find(L"System") != std::wstring::npos ||
                m_volumeType.find(L"SYSTEM") != std::wstring::npos;
        }

        [[nodiscard]] bool IsBootVolume() const noexcept {
            if (m_volumeType.empty()) return false;

            return m_volumeType.find(L"boot") != std::wstring::npos ||
                m_volumeType.find(L"Boot") != std::wstring::npos ||
                m_volumeType.find(L"BOOT") != std::wstring::npos;
        }

        [[nodiscard]] bool HasEnoughSpace(DiskSize required) const noexcept {
            return m_size >= required;
        }

        [[nodiscard]] double GetSizeGB() const noexcept {
            return m_size.ToGB();
        }

        static constexpr int INVALID_INDEX = -1;

    private:
        int m_index = INVALID_INDEX;
        std::wstring m_letter;
        std::wstring m_label;
        FileSystemType m_fileSystem = FileSystemType::Unknown;
        DiskSize m_size;
        std::wstring m_volumeType;
    };

}
