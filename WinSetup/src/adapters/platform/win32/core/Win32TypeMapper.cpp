// src/adapters/platform/win32/core/Win32TypeMapper.cpp
#include "Win32TypeMapper.h"
#include <cwctype>
#include <algorithm>

namespace winsetup::adapters::platform {

    namespace {

        constexpr GUID kPartitionSystemGuid =
        { 0xC12A7328, 0xF81F, 0x11D2, { 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B } };

        constexpr GUID kPartitionMsftReservedGuid =
        { 0xE3C9E316, 0x0B5C, 0x4DB8, { 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE } };

        constexpr GUID kPartitionBasicDataGuid =
        { 0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } };

        constexpr GUID kPartitionRecoveryGuid =
        { 0xDE94BBA4, 0x06D1, 0x4D40, { 0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC } };

        constexpr GUID kPartitionOemGuid =
        { 0x21686148, 0x6449, 0x6E6F, { 0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } };

        constexpr GUID kGuidNull = {};

        bool GuidEqual(const GUID& a, const GUID& b) noexcept {
            return memcmp(&a, &b, sizeof(GUID)) == 0;
        }

    }

    domain::BusType Win32TypeMapper::MapBusType(STORAGE_BUS_TYPE winBusType) noexcept {
        switch (winBusType) {
        case BusTypeScsi:            return domain::BusType::SCSI;
        case BusTypeAtapi:           return domain::BusType::ATAPI;
        case BusTypeAta:             return domain::BusType::ATA;
        case BusType1394:            return domain::BusType::IEEE1394;
        case BusTypeSsa:             return domain::BusType::SSA;
        case BusTypeFibre:           return domain::BusType::FibreChannel;
        case BusTypeUsb:             return domain::BusType::USB;
        case BusTypeRAID:            return domain::BusType::RAID;
        case BusTypeiScsi:           return domain::BusType::iSCSI;
        case BusTypeSas:             return domain::BusType::SAS;
        case BusTypeSata:            return domain::BusType::SATA;
        case BusTypeSd:              return domain::BusType::SD;
        case BusTypeMmc:             return domain::BusType::MMC;
        case BusTypeVirtual:         return domain::BusType::Virtual;
        case BusTypeFileBackedVirtual: return domain::BusType::FileBackedVirtual;
        case BusTypeSpaces:          return domain::BusType::Spaces;
        case BusTypeNvme:            return domain::BusType::NVME;
#ifdef BusTypeScm
        case BusTypeScm:             return domain::BusType::SCM;
#endif
#ifdef BusTypeUfs
        case BusTypeUfs:             return domain::BusType::UFS;
#endif
        default:                     return domain::BusType::Unknown;
        }
    }

    domain::DiskType Win32TypeMapper::MapDiskType(
        MEDIA_TYPE mediaType,
        STORAGE_BUS_TYPE busType,
        bool hasSeekPenalty
    ) noexcept {
        if (busType == BusTypeNvme)                                     return domain::DiskType::NVME;
        if (busType == BusTypeVirtual || busType == BusTypeFileBackedVirtual)
            return domain::DiskType::Virtual;
        if (busType == BusTypeUsb || busType == BusTypeSd || busType == BusTypeMmc)
            return domain::DiskType::Removable;
        switch (mediaType) {
        case FixedMedia:    return hasSeekPenalty ? domain::DiskType::HDD : domain::DiskType::SSD;
        case RemovableMedia: return domain::DiskType::Removable;
        default:            return hasSeekPenalty ? domain::DiskType::HDD : domain::DiskType::Unknown;
        }
    }

    domain::FileSystemType Win32TypeMapper::MapFileSystemType(
        const std::wstring& fileSystemName
    ) noexcept {
        if (fileSystemName.empty()) return domain::FileSystemType::Unknown;

        std::wstring fsUpper = fileSystemName;
        std::transform(fsUpper.begin(), fsUpper.end(), fsUpper.begin(),
            [](wchar_t c) { return static_cast<wchar_t>(std::towupper(static_cast<std::wint_t>(c))); });

        if (fsUpper == L"NTFS")                          return domain::FileSystemType::NTFS;
        if (fsUpper == L"FAT32")                         return domain::FileSystemType::FAT32;
        if (fsUpper == L"FAT" || fsUpper == L"FAT16"
            || fsUpper == L"FAT12")    return domain::FileSystemType::FAT;
        if (fsUpper == L"EXFAT")                         return domain::FileSystemType::exFAT;
        if (fsUpper == L"REFS")                          return domain::FileSystemType::ReFS;
        if (fsUpper == L"RAW")                           return domain::FileSystemType::RAW;
        return domain::FileSystemType::Unknown;
    }

    domain::PartitionType Win32TypeMapper::MapPartitionType(
        BYTE partitionType,
        bool isGPT
    ) noexcept {
        if (isGPT) return domain::PartitionType::Basic;
        switch (partitionType) {
        case PARTITION_ENTRY_UNUSED:                    return domain::PartitionType::Unknown;
        case PARTITION_IFS:
        case PARTITION_NTFT:                            return domain::PartitionType::Basic;
        case PARTITION_EXTENDED:
        case PARTITION_XINT13_EXTENDED:                 return domain::PartitionType::Basic;
        default:
            if (partitionType == 0x07)                  return domain::PartitionType::Basic;
            if (partitionType == 0xEF)                  return domain::PartitionType::EFI;
            if (partitionType == 0x27)                  return domain::PartitionType::Recovery;
            if (partitionType == 0x12 || partitionType == 0x84)
                return domain::PartitionType::OEM;
            if (partitionType == 0x01 || partitionType == 0x06)
                return domain::PartitionType::Basic;
            return domain::PartitionType::Unknown;
        }
    }

    domain::PartitionType Win32TypeMapper::FromGuid(const GUID& guid) noexcept {
        if (GuidEqual(guid, kPartitionSystemGuid))      return domain::PartitionType::EFI;
        if (GuidEqual(guid, kPartitionMsftReservedGuid)) return domain::PartitionType::MSR;
        if (GuidEqual(guid, kPartitionRecoveryGuid))    return domain::PartitionType::Recovery;
        if (GuidEqual(guid, kPartitionOemGuid))         return domain::PartitionType::OEM;
        if (GuidEqual(guid, kPartitionBasicDataGuid))   return domain::PartitionType::Basic;
        return domain::PartitionType::Unknown;
    }

    GUID Win32TypeMapper::ToGuid(domain::PartitionType partitionType) noexcept {
        switch (partitionType) {
        case domain::PartitionType::EFI:      return kPartitionSystemGuid;
        case domain::PartitionType::MSR:      return kPartitionMsftReservedGuid;
        case domain::PartitionType::Recovery: return kPartitionRecoveryGuid;
        case domain::PartitionType::OEM:      return kPartitionOemGuid;
        default:                              return kPartitionBasicDataGuid;
        }
    }

    STORAGE_BUS_TYPE Win32TypeMapper::MapToWin32BusType(domain::BusType busType) noexcept {
        switch (busType) {
        case domain::BusType::SCSI:            return BusTypeScsi;
        case domain::BusType::ATAPI:           return BusTypeAtapi;
        case domain::BusType::ATA:             return BusTypeAta;
        case domain::BusType::IEEE1394:        return BusType1394;
        case domain::BusType::SSA:             return BusTypeSsa;
        case domain::BusType::FibreChannel:    return BusTypeFibre;
        case domain::BusType::USB:             return BusTypeUsb;
        case domain::BusType::RAID:            return BusTypeRAID;
        case domain::BusType::iSCSI:           return BusTypeiScsi;
        case domain::BusType::SAS:             return BusTypeSas;
        case domain::BusType::SATA:            return BusTypeSata;
        case domain::BusType::SD:              return BusTypeSd;
        case domain::BusType::MMC:             return BusTypeMmc;
        case domain::BusType::Virtual:         return BusTypeVirtual;
        case domain::BusType::FileBackedVirtual: return BusTypeFileBackedVirtual;
        case domain::BusType::Spaces:          return BusTypeSpaces;
        case domain::BusType::NVME:            return BusTypeNvme;
#ifdef BusTypeScm
        case domain::BusType::SCM:             return BusTypeScm;
#endif
#ifdef BusTypeUfs
        case domain::BusType::UFS:             return BusTypeUfs;
#endif
        default:                               return BusTypeUnknown;
        }
    }

    MEDIA_TYPE Win32TypeMapper::MapToWin32MediaType(domain::DiskType diskType) noexcept {
        switch (diskType) {
        case domain::DiskType::HDD:
        case domain::DiskType::SSD:
        case domain::DiskType::NVME:    return FixedMedia;
        case domain::DiskType::Removable: return RemovableMedia;
        default:                        return Unknown;
        }
    }

} // namespace winsetup::adapters::platform
