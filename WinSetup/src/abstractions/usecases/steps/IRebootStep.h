#pragma once
#include "domain/primitives/Expected.h"

namespace winsetup {
    namespace abstractions {

        class IRebootStep {
        public:
            virtual ~IRebootStep() = default;
            [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
        };

    } // namespace abstractions
} // namespace winsetup
