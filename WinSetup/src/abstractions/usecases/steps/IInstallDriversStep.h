#pragma once
#include "domain/primitives/Expected.h"

namespace winsetup {
    namespace abstractions {

        class IInstallDriversStep {
        public:
            virtual ~IInstallDriversStep() = default;
            [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
        };

    } // namespace abstractions
} // namespace winsetup
