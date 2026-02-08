// src/domain/valueobjects/PartitionType.h

#pragma once

#include <string>

namespace winsetup::domain {

    enum class PartitionType {
        Unknown,
        System,
        EFI,
        MSR,
        Basic,
        Recovery,
        OEM
    };

    inline std::wstring PartitionTypeToString(PartitionType type) {
        switch (type) {
        case PartitionType::System:   return L"System";
        case PartitionType::EFI:      return L"EFI";
        case PartitionType::MSR:      return L"MSR";
        case PartitionType::Basic:    return L"Basic";
        case PartitionType::Recovery: return L"Recovery";
        case PartitionType::OEM:      return L"OEM";
        default:                      return L"Unknown";
        }
    }

}
