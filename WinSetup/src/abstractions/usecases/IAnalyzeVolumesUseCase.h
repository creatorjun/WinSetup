#pragma once
#include "domain/primitives/Expected.h"

namespace winsetup::abstractions {

    class IAnalyzeVolumesUseCase {
    public:
        virtual ~IAnalyzeVolumesUseCase() = default;
        [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
    };

} // namespace winsetup::abstractions
