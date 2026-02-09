// src/domain/entities/PartitionInfo.h
#pragma once

#include <string>
#include "../valueobjects/DiskSize.h"
#include "../valueobjects/FileSystemType.h"
#include "../valueobjects/PartitionType.h"
#include "../valueobjects/DriveLetter.h"

namespace winsetup::domain {

    class PartitionInfo {
    public:
        PartitionInfo() = default;

        PartitionInfo(
            uint32_t index,
            PartitionType type,
            DiskSize size,
            FileSystemType fileSystem
        )
            : m_index(index)
            , m_type(type)
            , m_size(size)
            , m_fileSystem(fileSystem)
        {
        }

        [[nodiscard]] uint32_t GetIndex() const noexcept { return m_index; }
        [[nodiscard]] PartitionType GetType() const noexcept { return m_type; }
        [[nodiscard]] DiskSize GetSize() const noexcept { return m_size; }
        [[nodiscard]] FileSystemType GetFileSystem() const noexcept { return m_fileSystem; }
        [[nodiscard]] const std::wstring& GetLabel() const noexcept { return m_label; }
        [[nodiscard]] const std::optional<DriveLetter>& GetDriveLetter() const noexcept { return m_driveLetter; }
        [[nodiscard]] bool IsActive() const noexcept { return m_isActive; }

        void SetLabel(const std::wstring& label) { m_label = label; }
        void SetDriveLetter(const DriveLetter& letter) { m_driveLetter = letter; }
        void SetActive(bool active) noexcept { m_isActive = active; }

        [[nodiscard]] bool CanContainWindows() const noexcept {
            return m_type == PartitionType::Basic &&
                IsWindowsCompatible(m_fileSystem) &&
                m_size >= DiskSize::FromGB(20);
        }

        [[nodiscard]] bool IsBootable() const noexcept {
            return IsBootPartition(m_type) && m_isActive;
        }

    private:
        uint32_t m_index = 0;
        PartitionType m_type = PartitionType::Unknown;
        DiskSize m_size;
        FileSystemType m_fileSystem = FileSystemType::Unknown;
        std::wstring m_label;
        std::optional<DriveLetter> m_driveLetter;
        bool m_isActive = false;
    };

}
