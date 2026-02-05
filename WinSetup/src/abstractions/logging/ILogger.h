#pragma once

#include <string>
#include <string_view>
#include "../../domain/primitives/LogLevel.h"

namespace winsetup::abstractions {

    class ILogger {
    public:
        virtual ~ILogger() = default;

        virtual void Log(
            domain::LogLevel level,
            std::wstring_view message
        ) noexcept = 0;

        virtual void Log(
            domain::LogLevel level,
            std::wstring_view message,
            std::wstring_view category
        ) noexcept = 0;

        virtual void SetMinimumLevel(domain::LogLevel level) noexcept = 0;
        [[nodiscard]] virtual domain::LogLevel GetMinimumLevel() const noexcept = 0;

        virtual void Flush() noexcept = 0;
        virtual void Close() noexcept = 0;
    };

}
