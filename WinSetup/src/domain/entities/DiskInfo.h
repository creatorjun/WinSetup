#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <compare>
#include <domain/primitives/Expected.h>

namespace winsetup::domain {

    enum class BusType : uint8_t {
        Unknown = 0,
        SCSI = 1,
        ATAPI = 2,
        ATA = 3,
        IEEE1394 = 4,
        SSA = 5,
        FibreChannel = 6,
        USB = 7,
        RAID = 8,
        iSCSI = 9,
        SAS = 10,
        SATA = 11,
        SD = 12,
        MMC = 13,
        Virtual = 14,
        FileBackedVirtual = 15,
        Spaces = 16,
        NVMe = 17,
        SCM = 18,
        UFS = 19,
        Max = 20
    };

    enum class PartitionStyle : uint8_t {
        MBR,
        GPT,
        Raw
    };

    [[nodiscard]] constexpr const wchar_t* BusTypeToString(BusType type) noexcept {
        switch (type) {
        case BusType::NVMe:    return L"NVMe";
        case BusType::SATA:    return L"SATA";
        case BusType::SAS:     return L"SAS";
        case BusType::USB:     return L"USB";
        case BusType::RAID:    return L"RAID";
        case BusType::Virtual: return L"Virtual";
        default:               return L"Unknown";
        }
    }

    [[nodiscard]] constexpr int GetBusPriority(BusType type) noexcept {
        switch (type) {
        case BusType::NVMe:    return 100;
        case BusType::SAS:     return 90;
        case BusType::RAID:    return 85;
        case BusType::SATA:    return 80;
        case BusType::SCSI:    return 70;
        case BusType::ATA:     return 60;
        case BusType::USB:     return 10;
        default:               return 0;
        }
    }

    struct PhysicalDiskId {
        uint32_t index;

        [[nodiscard]] constexpr auto operator<=>(const PhysicalDiskId&) const = default;

        [[nodiscard]] std::wstring ToString() const {
            return L"\\\\.\\PhysicalDrive" + std::to_wstring(index);
        }
    };

    class DiskInfo {
    public:
        DiskInfo() = default;

        DiskInfo(PhysicalDiskId id, BusType busType, uint64_t size, std::wstring model)
            : id_(id)
            , busType_(busType)
            , sizeInBytes_(size)
            , model_(std::move(model))
            , partitionStyle_(PartitionStyle::Raw) {
        }

        [[nodiscard]] PhysicalDiskId GetId() const noexcept {
            return id_;
        }

        [[nodiscard]] BusType GetBusType() const noexcept {
            return busType_;
        }

        [[nodiscard]] uint64_t GetSizeInBytes() const noexcept {
            return sizeInBytes_;
        }

        [[nodiscard]] uint64_t GetSizeInGB() const noexcept {
            return sizeInBytes_ / (1024ULL * 1024ULL * 1024ULL);
        }

        [[nodiscard]] const std::wstring& GetModel() const noexcept {
            return model_;
        }

        [[nodiscard]] PartitionStyle GetPartitionStyle() const noexcept {
            return partitionStyle_;
        }

        void SetPartitionStyle(PartitionStyle style) noexcept {
            partitionStyle_ = style;
        }

        [[nodiscard]] bool IsRemovable() const noexcept {
            return busType_ == BusType::USB;
        }

        [[nodiscard]] int GetPriority() const noexcept {
            return GetBusPriority(busType_);
        }

        [[nodiscard]] std::strong_ordering CompareForInstallation(const DiskInfo& other) const noexcept {
            const int thisPriority = GetPriority();
            const int otherPriority = other.GetPriority();

            if (thisPriority != otherPriority) {
                return thisPriority <=> otherPriority;
            }

            return sizeInBytes_ <=> other.sizeInBytes_;
        }

    private:
        PhysicalDiskId id_;
        BusType busType_{ BusType::Unknown };
        uint64_t sizeInBytes_{ 0 };
        std::wstring model_;
        PartitionStyle partitionStyle_{ PartitionStyle::Raw };
    };

}
