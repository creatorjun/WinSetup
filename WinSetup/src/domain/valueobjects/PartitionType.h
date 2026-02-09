// src/domain/valueobjects/PartitionType.h
#pragma once

#include <string>

namespace winsetup::domain {

    enum class PartitionType {
        Unknown = 0,
        System = 1,
        EFI = 2,
        MSR = 3,
        Basic = 4,
        Recovery = 5,
        OEM = 6
    };

    [[nodiscard]] inline std::wstring PartitionTypeToString(PartitionType type) noexcept {
        switch (type) {
        case PartitionType::System: return L"System";
        case PartitionType::EFI: return L"EFI";
        case PartitionType::MSR: return L"MSR";
        case PartitionType::Basic: return L"Basic";
        case PartitionType::Recovery: return L"Recovery";
        case PartitionType::OEM: return L"OEM";
        default: return L"Unknown";
        }
    }

}
