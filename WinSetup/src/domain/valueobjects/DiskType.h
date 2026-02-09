// src/domain/valueobjects/DiskType.h
#pragma once

#include <string>

namespace winsetup::domain {

    enum class DiskType {
        Unknown = 0,
        HDD = 1,
        SSD = 2,
        Removable = 3
    };

    [[nodiscard]] inline std::wstring DiskTypeToString(DiskType type) noexcept {
        switch (type) {
        case DiskType::HDD: return L"HDD";
        case DiskType::SSD: return L"SSD";
        case DiskType::Removable: return L"Removable";
        default: return L"Unknown";
        }
    }

    [[nodiscard]] inline DiskType ParseDiskType(const std::wstring& typeStr) noexcept {
        std::wstring upper = typeStr;
        for (auto& c : upper) {
            c = static_cast<wchar_t>(::towupper(c));
        }

        if (upper.find(L"SSD") != std::wstring::npos) return DiskType::SSD;
        if (upper.find(L"HDD") != std::wstring::npos) return DiskType::HDD;
        if (upper.find(L"REMOVABLE") != std::wstring::npos) return DiskType::Removable;

        return DiskType::Unknown;
    }

}
