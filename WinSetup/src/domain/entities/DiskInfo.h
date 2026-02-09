// src/domain/entities/DiskInfo.h
#pragma once

#include <string>
#include <vector>
#include "../valueobjects/DiskSize.h"
#include "../valueobjects/BusType.h"
#include "../valueobjects/DiskType.h"
#include "PartitionInfo.h"

namespace winsetup::domain {

    class DiskInfo {
    public:
        DiskInfo() = default;

        DiskInfo(
            uint32_t index,
            DiskSize size,
            BusType busType,
            DiskType diskType
        )
            : m_index(index)
            , m_size(size)
            , m_busType(busType)
            , m_diskType(diskType)
        {
        }

        [[nodiscard]] uint32_t GetIndex() const noexcept { return m_index; }
        [[nodiscard]] DiskSize GetSize() const noexcept { return m_size; }
        [[nodiscard]] BusType GetBusType() const noexcept { return m_busType; }
        [[nodiscard]] DiskType GetDiskType() const noexcept { return m_diskType; }
        [[nodiscard]] const std::wstring& GetModel() const noexcept { return m_model; }
        [[nodiscard]] const std::wstring& GetSerialNumber() const noexcept { return m_serialNumber; }
        [[nodiscard]] const std::vector<PartitionInfo>& GetPartitions() const noexcept { return m_partitions; }
        [[nodiscard]] bool IsRemovable() const noexcept { return m_isRemovable; }
        [[nodiscard]] bool IsBootDrive() const noexcept { return m_isBootDrive; }

        void SetIndex(uint32_t index) noexcept { m_index = index; }
        void SetSize(DiskSize size) noexcept { m_size = size; }
        void SetBusType(BusType type) noexcept { m_busType = type; }
        void SetDiskType(DiskType type) noexcept { m_diskType = type; }
        void SetType(DiskType type) noexcept { m_diskType = type; }
        void SetModel(const std::wstring& model) { m_model = model; }
        void SetModel(std::wstring&& model) noexcept { m_model = std::move(model); }
        void SetSerialNumber(const std::wstring& serial) { m_serialNumber = serial; }
        void SetSerialNumber(std::wstring&& serial) noexcept { m_serialNumber = std::move(serial); }
        void SetPartitions(const std::vector<PartitionInfo>& partitions) { m_partitions = partitions; }
        void SetPartitions(std::vector<PartitionInfo>&& partitions) noexcept { m_partitions = std::move(partitions); }
        void SetRemovable(bool removable) noexcept { m_isRemovable = removable; }
        void SetBootDrive(bool bootDrive) noexcept { m_isBootDrive = bootDrive; }

        void AddPartition(const PartitionInfo& partition) { m_partitions.push_back(partition); }
        void AddPartition(PartitionInfo&& partition) { m_partitions.push_back(std::move(partition)); }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_size.ToBytes() > 0;
        }

        [[nodiscard]] bool IsSSD() const noexcept {
            return m_diskType == DiskType::SSD;
        }

        [[nodiscard]] bool IsHDD() const noexcept {
            return m_diskType == DiskType::HDD;
        }

        [[nodiscard]] bool IsNVMe() const noexcept {
            return m_busType == BusType::NVME;
        }

        [[nodiscard]] bool HasPartitions() const noexcept {
            return !m_partitions.empty();
        }

        [[nodiscard]] bool HasEnoughSpace(DiskSize required) const noexcept {
            return m_size >= required;
        }

        [[nodiscard]] double GetSizeGB() const noexcept {
            return m_size.ToGB();
        }

    private:
        uint32_t m_index = 0;
        DiskSize m_size;
        BusType m_busType = BusType::Unknown;
        DiskType m_diskType = DiskType::Unknown;
        std::wstring m_model;
        std::wstring m_serialNumber;
        std::vector<PartitionInfo> m_partitions;
        bool m_isRemovable = false;
        bool m_isBootDrive = false;
    };

}
