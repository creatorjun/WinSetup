#pragma once

#include <string>
#include "../../domain/primitives/Expected.h"
#include "../../domain/primitives/Error.h"

namespace winsetup::abstractions {

    struct SystemBoardInfo {
        std::wstring manufacturer;
        std::wstring product;
        std::wstring version;
        std::wstring serialNumber;

        [[nodiscard]] std::wstring GetDisplayName() const noexcept {
            if (!product.empty()) {
                return product;
            }
            if (!manufacturer.empty()) {
                return manufacturer;
            }
            return L"Unknown";
        }
    };

    struct SystemInfo {
        std::wstring manufacturer;
        std::wstring productName;
        std::wstring version;
        SystemBoardInfo baseboard;

        [[nodiscard]] std::wstring GetMotherboardModel() const noexcept {
            return baseboard.GetDisplayName();
        }
    };

    class ISystemInfoService {
    public:
        virtual ~ISystemInfoService() = default;

        [[nodiscard]] virtual domain::Expected<SystemInfo>
            GetSystemInfo() const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<SystemBoardInfo>
            GetBaseboardInfo() const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<std::wstring>
            GetMotherboardModel() const noexcept = 0;
    };

}
