// src/abstractions/usecases/IAnalyzeSystemUseCase.h
#pragma once

#include <domain/primitives/Expected.h>

namespace winsetup::abstractions {

    class IAnalyzeSystemUseCase {
    public:
        virtual ~IAnalyzeSystemUseCase() = default;

        [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
    };

}
