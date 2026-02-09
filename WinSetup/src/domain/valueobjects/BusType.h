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

    [[nodiscard]] inline std::wstring BusTypeToString(BusType busType) noexcept {
        switch (busType) {
        case BusType::NVME: return L"NVMe";
        case BusType::SATA: return L"SATA";
        case BusType::USB: return L"USB";
        case BusType::RAID: return L"RAID";
        case BusType::SCSI: return L"SCSI";
        case BusType::SAS: return L"SAS";
        default: return L"UNKNOWN";
        }
    }

}
