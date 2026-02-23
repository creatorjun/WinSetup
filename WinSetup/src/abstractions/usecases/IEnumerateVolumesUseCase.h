// src/abstractions/usecases/IEnumerateVolumesUseCase.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::abstractions {

    class IEnumerateVolumesUseCase {
    public:
        virtual ~IEnumerateVolumesUseCase() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<std::vector<domain::VolumeInfo>>>
            Execute() = 0;
    };

}
