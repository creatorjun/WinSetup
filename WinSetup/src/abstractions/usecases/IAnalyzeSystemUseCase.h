// src/abstractions/usecases/IAnalyzeSystemUseCase.h
#pragma once
#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <memory>

namespace winsetup::abstractions {

    class IAnalyzeSystemUseCase {
    public:
        virtual ~IAnalyzeSystemUseCase() = default;
        [[nodiscard]] virtual domain::Expected<std::shared_ptr<domain::SystemInfo>>
            Execute() = 0;
    };

} // namespace winsetup::abstractions
