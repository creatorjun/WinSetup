// src/abstractions/infrastructure/logging/LogLevel.h

#pragma once

namespace winsetup::abstractions {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

}
