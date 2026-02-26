// src/abstractions/usecases/IAnalyzeDisksUseCase.h
#pragma once
#include <domain/primitives/Expected.h>

namespace winsetup::abstractions {

    class IAnalyzeDisksUseCase {
    public:
        virtual ~IAnalyzeDisksUseCase() = default;
        [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
    };

}
