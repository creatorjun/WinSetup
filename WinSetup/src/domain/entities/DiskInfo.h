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
        DiskInfo(uint32_t index, DiskSize size, BusType busType, DiskType diskType)
            : mIndex(index), mSize(size), mBusType(busType), mDiskType(diskType)
        {
        }

        [[nodiscard]] uint32_t                          GetIndex()        const noexcept { return mIndex; }
        [[nodiscard]] DiskSize                          GetSize()         const noexcept { return mSize; }
        [[nodiscard]] BusType                           GetBusType()      const noexcept { return mBusType; }
        [[nodiscard]] DiskType                          GetDiskType()     const noexcept { return mDiskType; }
        [[nodiscard]] const std::wstring& GetModel()        const noexcept { return mModel; }
        [[nodiscard]] const std::wstring& GetSerialNumber() const noexcept { return mSerialNumber; }
        [[nodiscard]] const std::vector<PartitionInfo>& GetPartitions()   const noexcept { return mPartitions; }
        [[nodiscard]] bool                              IsRemovable()     const noexcept { return mIsRemovable; }
        [[nodiscard]] bool                              IsSystem()        const noexcept { return mIsSystem; }
        [[nodiscard]] bool                              IsData()          const noexcept { return mIsData; }

        [[nodiscard]] bool IsValid()        const noexcept { return mSize.ToBytes() > 0; }
        [[nodiscard]] bool IsSSD()          const noexcept { return mDiskType == DiskType::SSD; }
        [[nodiscard]] bool IsHDD()          const noexcept { return mDiskType == DiskType::HDD; }
        [[nodiscard]] bool IsNVMe()         const noexcept { return mDiskType == DiskType::NVME; }
        [[nodiscard]] bool IsVirtual()      const noexcept { return mDiskType == DiskType::Virtual; }
        [[nodiscard]] bool IsSolidState()   const noexcept { return mDiskType == DiskType::SSD || mDiskType == DiskType::NVME; }
        [[nodiscard]] bool IsPhysicalDisk() const noexcept {
            return mDiskType == DiskType::HDD
                || mDiskType == DiskType::SSD
                || mDiskType == DiskType::NVME;
        }
        [[nodiscard]] bool HasPartitions()               const noexcept { return !mPartitions.empty(); }
        [[nodiscard]] bool HasEnoughSpace(DiskSize req)  const noexcept { return mSize >= req; }
        [[nodiscard]] double GetSizeGB()                 const noexcept { return mSize.ToGB(); }

        void SetIndex(uint32_t index)                           noexcept { mIndex = index; }
        void SetSize(DiskSize size)                             noexcept { mSize = size; }
        void SetBusType(BusType type)                           noexcept { mBusType = type; }
        void SetDiskType(DiskType type)                         noexcept { mDiskType = type; }
        void SetType(DiskType type)                             noexcept { mDiskType = type; }
        void SetModel(const std::wstring& model) { mModel = model; }
        void SetModel(std::wstring&& model)                     noexcept { mModel = std::move(model); }
        void SetSerialNumber(const std::wstring& serial) { mSerialNumber = serial; }
        void SetSerialNumber(std::wstring&& serial)             noexcept { mSerialNumber = std::move(serial); }
        void SetPartitions(const std::vector<PartitionInfo>& p) { mPartitions = p; }
        void SetPartitions(std::vector<PartitionInfo>&& p)      noexcept { mPartitions = std::move(p); }
        void SetRemovable(bool v)                               noexcept { mIsRemovable = v; }
        void SetIsSystem(bool v)                                noexcept { mIsSystem = v; }
        void SetIsData(bool v)                                  noexcept { mIsData = v; }
        void AddPartition(const PartitionInfo& p) { mPartitions.push_back(p); }

    private:
        uint32_t                   mIndex = 0;
        DiskSize                   mSize;
        BusType                    mBusType = BusType::Unknown;
        DiskType                   mDiskType = DiskType::Unknown;
        std::wstring               mModel;
        std::wstring               mSerialNumber;
        std::vector<PartitionInfo> mPartitions;
        bool                       mIsRemovable = false;
        bool                       mIsSystem = false;
        bool                       mIsData = false;
    };

}
