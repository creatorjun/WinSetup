// src/abstractions/usecases/steps/IAnalyzeDisksStep.h
#pragma once
#include "domain/primitives/Expected.h"

namespace winsetup::abstractions {

    class IAnalyzeDisksStep {
    public:
        virtual ~IAnalyzeDisksStep() = default;
        [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
    };

} // namespace winsetup::abstractions
