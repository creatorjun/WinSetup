// src/abstractions/usecases/IAnalyzeSystemUseCase.h
#pragma once

#include <domain/primitives/Expected.h>
#include <application/dto/SystemAnalysisResult.h>
#include <memory>

namespace winsetup::abstractions {

    class IAnalyzeSystemUseCase {
    public:
        virtual ~IAnalyzeSystemUseCase() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<application::SystemAnalysisResult>>
            Execute() = 0;
    };

}
