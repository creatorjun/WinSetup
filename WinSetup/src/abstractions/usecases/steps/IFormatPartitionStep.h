#pragma once
#include "domain/primitives/Expected.h"

namespace winsetup {
    namespace abstractions {

        class IFormatPartitionStep {
        public:
            virtual ~IFormatPartitionStep() = default;
            [[nodiscard]] virtual domain::Expected<void> Execute() = 0;
        };

    } // namespace abstractions
} // namespace winsetup
