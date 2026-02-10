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
            : mindex(index)
            , msize(size)
            , mbusType(busType)
            , mdiskType(diskType)
        {
        }

        [[nodiscard]] uint32_t GetIndex() const noexcept { return mindex; }
        [[nodiscard]] DiskSize GetSize() const noexcept { return msize; }
        [[nodiscard]] BusType GetBusType() const noexcept { return mbusType; }
        [[nodiscard]] DiskType GetDiskType() const noexcept { return mdiskType; }
        [[nodiscard]] const std::wstring& GetModel() const noexcept { return mmodel; }
        [[nodiscard]] const std::wstring& GetSerialNumber() const noexcept { return mserialNumber; }
        [[nodiscard]] const std::vector<PartitionInfo>& GetPartitions() const noexcept { return mpartitions; }
        [[nodiscard]] bool IsRemovable() const noexcept { return misRemovable; }
        [[nodiscard]] bool IsBootDrive() const noexcept { return misBootDrive; }

        void SetIndex(uint32_t index) noexcept { mindex = index; }
        void SetSize(DiskSize size) noexcept { msize = size; }
        void SetBusType(BusType type) noexcept { mbusType = type; }
        void SetDiskType(DiskType type) noexcept { mdiskType = type; }
        void SetType(DiskType type) noexcept { mdiskType = type; }
        void SetModel(const std::wstring& model) { mmodel = model; }
        void SetModel(std::wstring&& model) noexcept { mmodel = std::move(model); }
        void SetSerialNumber(const std::wstring& serial) { mserialNumber = serial; }
        void SetSerialNumber(std::wstring&& serial) noexcept { mserialNumber = std::move(serial); }
        void SetPartitions(const std::vector<PartitionInfo>& partitions) { mpartitions = partitions; }
        void SetPartitions(std::vector<PartitionInfo>&& partitions) noexcept { mpartitions = std::move(partitions); }
        void SetRemovable(bool removable) noexcept { misRemovable = removable; }
        void SetBootDrive(bool bootDrive) noexcept { misBootDrive = bootDrive; }

        void AddPartition(const PartitionInfo& partition) { mpartitions.push_back(partition); }
        void AddPartition(PartitionInfo&& partition) { mpartitions.push_back(std::move(partition)); }

        [[nodiscard]] bool IsValid() const noexcept {
            return msize.ToBytes() > 0;
        }

        [[nodiscard]] bool IsSSD() const noexcept {
            return mdiskType == DiskType::SSD;
        }

        [[nodiscard]] bool IsHDD() const noexcept {
            return mdiskType == DiskType::HDD;
        }

        [[nodiscard]] bool IsNVMe() const noexcept {
            return mdiskType == DiskType::NVME;
        }

        [[nodiscard]] bool IsVirtual() const noexcept {
            return mdiskType == DiskType::Virtual;
        }

        [[nodiscard]] bool IsSolidState() const noexcept {
            return mdiskType == DiskType::SSD || mdiskType == DiskType::NVME;
        }

        [[nodiscard]] bool IsPhysicalDisk() const noexcept {
            return mdiskType == DiskType::HDD ||
                mdiskType == DiskType::SSD ||
                mdiskType == DiskType::NVME;
        }

        [[nodiscard]] bool HasPartitions() const noexcept {
            return !mpartitions.empty();
        }

        [[nodiscard]] bool HasEnoughSpace(DiskSize required) const noexcept {
            return msize >= required;
        }

        [[nodiscard]] double GetSizeGB() const noexcept {
            return msize.ToGB();
        }

    private:
        uint32_t mindex = 0;
        DiskSize msize;
        BusType mbusType = BusType::Unknown;
        DiskType mdiskType = DiskType::Unknown;
        std::wstring mmodel;
        std::wstring mserialNumber;
        std::vector<PartitionInfo> mpartitions;
        bool misRemovable = false;
        bool misBootDrive = false;
    };

}
