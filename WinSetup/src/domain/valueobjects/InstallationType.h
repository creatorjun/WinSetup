// src/domain/valueobjects/InstallationType.h
#pragma once

#include <string>

namespace winsetup::domain {

    struct InstallationType {
        std::wstring name;
        std::wstring description;

        InstallationType() = default;
        InstallationType(const std::wstring& n, const std::wstring& d)
            : name(n), description(d) {
        }
    };

}
