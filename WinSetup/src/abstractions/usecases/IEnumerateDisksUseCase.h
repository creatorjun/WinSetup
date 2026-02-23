// src/abstractions/usecases/IEnumerateDisksUseCase.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/DiskInfo.h>
#include <memory>
#include <vector>

namespace winsetup::abstractions {

    class IEnumerateDisksUseCase {
    public:
        virtual ~IEnumerateDisksUseCase() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
            Execute() = 0;
    };

}
