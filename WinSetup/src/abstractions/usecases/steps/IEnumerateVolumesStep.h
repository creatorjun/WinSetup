// src/abstractions/usecases/steps/IEnumerateVolumesStep.h
#pragma once
#include "domain/primitives/Expected.h"
#include "domain/entities/VolumeInfo.h"
#include <memory>
#include <vector>

namespace winsetup::abstractions {

    class IEnumerateVolumesStep {
    public:
        virtual ~IEnumerateVolumesStep() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<std::vector<domain::VolumeInfo>>>
            Execute() = 0;
    };

} // namespace winsetup::abstractions
