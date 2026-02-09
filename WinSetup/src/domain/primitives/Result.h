// src/domain/primitives/Result.h
#pragma once

#include "Expected.h"

namespace winsetup::domain {

    template<typename T>
    using Result = Expected<T>;

}
