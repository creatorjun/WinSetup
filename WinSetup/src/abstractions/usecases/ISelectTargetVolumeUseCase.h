// src/abstractions/usecases/ISelectTargetVolumeUseCase.h
#pragma once
#include <domain/primitives/Expected.h>

namespace winsetup::abstractions {

    class ISelectTargetVolumeUseCase {
    public:
        virtual ~ISelectTargetVolumeUseCase() = default;
        [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
    };

}
