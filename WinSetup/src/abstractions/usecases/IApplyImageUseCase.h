#pragma once
#include "domain/primitives/Expected.h"
#include "domain/entities/SetupConfig.h"
#include <memory>

namespace winsetup::abstractions {

    class IApplyImageUseCase {
    public:
        virtual ~IApplyImageUseCase() = default;

        [[nodiscard]] virtual domain::Expected<void> Execute(
            std::shared_ptr<const domain::SetupConfig> config) = 0;
    };

} // namespace winsetup::abstractions
