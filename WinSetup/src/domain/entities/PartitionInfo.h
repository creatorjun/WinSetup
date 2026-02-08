// src/domain/entities/PartitionInfo.h

#pragma once

#include <string>
#include <domain/valueobjects/PartitionType.h>
#include <domain/valueobjects/FileSystemType.h>
#include <domain/valueobjects/DiskSize.h>

namespace winsetup::domain {

    class PartitionInfo {
    public:
        PartitionInfo() = default;

        PartitionInfo(
            int index,
            PartitionType type,
            DiskSize size,
            uint64_t offset = 0
        )
            : m_index(index)
            , m_type(type)
            , m_size(size)
            , m_offset(offset)
        {
        }

        [[nodiscard]] int GetIndex() const noexcept { return m_index; }
        [[nodiscard]] PartitionType GetType() const noexcept { return m_type; }
        [[nodiscard]] DiskSize GetSize() const noexcept { return m_size; }
        [[nodiscard]] uint64_t GetOffset() const noexcept { return m_offset; }
        [[nodiscard]] FileSystemType GetFileSystem() const noexcept { return m_fileSystem; }
        [[nodiscard]] const std::wstring& GetLabel() const noexcept { return m_label; }

        void SetIndex(int index) noexcept { m_index = index; }
        void SetType(PartitionType type) noexcept { m_type = type; }
        void SetSize(DiskSize size) noexcept { m_size = size; }
        void SetOffset(uint64_t offset) noexcept { m_offset = offset; }
        void SetFileSystem(FileSystemType fs) noexcept { m_fileSystem = fs; }
        void SetLabel(const std::wstring& label) { m_label = label; }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_index >= 0 && m_size.ToBytes() > 0;
        }

        [[nodiscard]] bool IsSystemPartition() const noexcept {
            return m_type == PartitionType::System ||
                m_type == PartitionType::EFI;
        }

        [[nodiscard]] double GetSizeGB() const noexcept {
            return m_size.ToGB();
        }

        static constexpr int INVALID_INDEX = -1;

    private:
        int m_index = INVALID_INDEX;
        PartitionType m_type = PartitionType::Unknown;
        DiskSize m_size;
        uint64_t m_offset = 0;
        FileSystemType m_fileSystem = FileSystemType::Unknown;
        std::wstring m_label;
    };

}
