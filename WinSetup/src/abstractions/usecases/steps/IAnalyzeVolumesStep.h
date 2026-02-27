// src/abstractions/usecases/steps/IAnalyzeVolumesStep.h
#pragma once
#include "domain/primitives/Expected.h"

namespace winsetup::abstractions {

    class IAnalyzeVolumesStep {
    public:
        virtual ~IAnalyzeVolumesStep() = default;
        [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
    };

} // namespace winsetup::abstractions
