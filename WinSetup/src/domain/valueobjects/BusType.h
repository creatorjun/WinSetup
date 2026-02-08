// src/domain/valueobjects/BusType.h

#pragma once

#include <string>

namespace winsetup::domain {

    enum class BusType {
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
        NVME = 17,
        SCM = 18,
        UFS = 19,
        Max = 20
    };

    enum class DiskTypeEnum {
        NVME = 4,
        SSD = 3,
        SATA = 2,
        HDD = 1,
        UNKNOWN = 0
    };

    inline int GetDiskTypePriority(DiskTypeEnum type) noexcept {
        return static_cast<int>(type);
    }

    inline std::wstring BusTypeToString(BusType busType) {
        switch (busType) {
        case BusType::NVME: return L"NVME";
        case BusType::SATA: return L"SATA";
        case BusType::USB: return L"USB";
        case BusType::RAID: return L"RAID";
        case BusType::SCSI: return L"SCSI";
        case BusType::SAS: return L"SAS";
        default: return L"UNKNOWN";
        }
    }

    inline std::wstring DiskTypeEnumToString(DiskTypeEnum type) {
        switch (type) {
        case DiskTypeEnum::NVME: return L"NVME";
        case DiskTypeEnum::SSD: return L"SSD";
        case DiskTypeEnum::SATA: return L"SATA";
        case DiskTypeEnum::HDD: return L"HDD";
        default: return L"UNKNOWN";
        }
    }

    inline DiskTypeEnum ParseDiskType(const std::wstring& typeStr) {
        std::wstring upper = typeStr;
        for (auto& c : upper) {
            c = static_cast<wchar_t>(::towupper(c));
        }

        if (upper.find(L"NVME") != std::wstring::npos) return DiskTypeEnum::NVME;
        if (upper.find(L"SSD") != std::wstring::npos) return DiskTypeEnum::SSD;
        if (upper.find(L"SATA") != std::wstring::npos) return DiskTypeEnum::SATA;
        if (upper.find(L"HDD") != std::wstring::npos) return DiskTypeEnum::HDD;

        return DiskTypeEnum::UNKNOWN;
    }

}
