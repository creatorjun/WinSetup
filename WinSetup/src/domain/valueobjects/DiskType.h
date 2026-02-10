// src/domain/valueobjects/DiskType.h
#pragma once

#include <string>
#include <cwctype>

namespace winsetup::domain {

    enum class DiskType {
        Unknown = 0,
        HDD = 1,
        SSD = 2,
        NVME = 3,
        Removable = 4,
        Virtual = 5
    };

    [[nodiscard]] inline std::wstring DiskTypeToString(DiskType type) noexcept {
        switch (type) {
        case DiskType::HDD:
            return L"HDD";
        case DiskType::SSD:
            return L"SSD";
        case DiskType::NVME:
            return L"NVMe";
        case DiskType::Removable:
            return L"Removable";
        case DiskType::Virtual:
            return L"Virtual";
        default:
            return L"Unknown";
        }
    }

    [[nodiscard]] inline DiskType ParseDiskType(const std::wstring& typeStr) noexcept {
        if (typeStr.empty()) {
            return DiskType::Unknown;
        }

        std::wstring upper = typeStr;
        for (auto& c : upper) {
            c = static_cast<wchar_t>(std::towupper(static_cast<std::wint_t>(c)));
        }

        if (upper.find(L"NVME") != std::wstring::npos) {
            return DiskType::NVME;
        }
        if (upper.find(L"SSD") != std::wstring::npos) {
            return DiskType::SSD;
        }
        if (upper.find(L"HDD") != std::wstring::npos) {
            return DiskType::HDD;
        }
        if (upper.find(L"REMOVABLE") != std::wstring::npos) {
            return DiskType::Removable;
        }
        if (upper.find(L"VIRTUAL") != std::wstring::npos) {
            return DiskType::Virtual;
        }

        return DiskType::Unknown;
    }

    [[nodiscard]] inline bool IsSolidState(DiskType type) noexcept {
        return type == DiskType::SSD || type == DiskType::NVME;
    }

    [[nodiscard]] inline bool IsPhysicalDisk(DiskType type) noexcept {
        return type == DiskType::HDD ||
            type == DiskType::SSD ||
            type == DiskType::NVME;
    }

    [[nodiscard]] inline bool IsRemovableDisk(DiskType type) noexcept {
        return type == DiskType::Removable;
    }

    [[nodiscard]] inline bool IsVirtualDisk(DiskType type) noexcept {
        return type == DiskType::Virtual;
    }

    [[nodiscard]] inline int GetDiskTypePriority(DiskType type) noexcept {
        switch (type) {
        case DiskType::NVME:
            return 10;
        case DiskType::SSD:
            return 20;
        case DiskType::HDD:
            return 30;
        case DiskType::Virtual:
            return 40;
        case DiskType::Removable:
            return 50;
        default:
            return 100;
        }
    }

}
