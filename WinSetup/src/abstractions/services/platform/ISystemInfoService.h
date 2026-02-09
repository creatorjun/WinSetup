// src/abstractions/services/platform/ISystemInfoService.h
#pragma once

#include "../../../domain/primitives/Expected.h"
#include "../../../domain/entities/SystemInfo.h"
#include <string>

namespace winsetup::abstractions {

    class ISystemInfoService {
    public:
        virtual ~ISystemInfoService() = default;

        [[nodiscard]] virtual domain::Expected<std::wstring>
            GetMotherboardModel() = 0;

        [[nodiscard]] virtual domain::Expected<std::wstring>
            GetBIOSVersion() = 0;

        [[nodiscard]] virtual domain::Expected<bool>
            IsUEFIBoot() = 0;

        [[nodiscard]] virtual domain::Expected<uint64_t>
            GetTotalMemoryBytes() = 0;
    };

}
