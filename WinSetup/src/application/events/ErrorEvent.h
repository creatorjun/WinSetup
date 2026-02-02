// src/application/events/ErrorEvent.h 
#pragma once

#include <string>
#include <chrono>
#include "../../domain/primitives/Error.h"

namespace winsetup::application {

    enum class ErrorSeverity {
        Info,
        Warning,
        Error,
        Critical
    };

    struct ErrorEvent {
        winsetup::domain::Error error;
        ErrorSeverity severity;
        std::string context;
        std::chrono::system_clock::time_point timestamp;
        bool recoverable;

        ErrorEvent()
            : error("Unknown error")
            , severity(ErrorSeverity::Error)
            , timestamp(std::chrono::system_clock::now())
            , recoverable(false) {
        }

        ErrorEvent(
            winsetup::domain::Error err,
            ErrorSeverity sev = ErrorSeverity::Error,
            std::string ctx = "",
            bool recover = false
        ) : error(std::move(err))
            , severity(sev)
            , context(std::move(ctx))
            , timestamp(std::chrono::system_clock::now())
            , recoverable(recover) {
        }

        bool IsCritical() const noexcept {
            return severity == ErrorSeverity::Critical;
        }
    };

}
