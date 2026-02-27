// src/abstractions/usecases/steps/IEnumerateDisksStep.h
#pragma once
#include "domain/primitives/Expected.h"
#include "domain/entities/DiskInfo.h"
#include <memory>
#include <vector>

namespace winsetup::abstractions {

    class IEnumerateDisksStep {
    public:
        virtual ~IEnumerateDisksStep() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
            Execute() = 0;
    };

} // namespace winsetup::abstractions
