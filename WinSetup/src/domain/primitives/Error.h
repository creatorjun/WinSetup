#pragma once

#include <string>
#include <optional>
#include <format>

namespace winsetup::domain {

    class Error {
    public:
        explicit Error(std::string message)
            : message_(std::move(message)), code_(std::nullopt) {
        }

        Error(std::string message, int code)
            : message_(std::move(message)), code_(code) {
        }

        const std::string& Message() const noexcept {
            return message_;
        }

        int Code() const noexcept {
            return code_.value_or(0);
        }

        bool HasCode() const noexcept {
            return code_.has_value();
        }

        static Error FromWin32(unsigned long errorCode) {
            return Error(
                std::format("Win32 error: 0x{:08X}", errorCode),
                static_cast<int>(errorCode)
            );
        }

        static Error FromHRESULT(long hr) {
            return Error(
                std::format("HRESULT: 0x{:08X}", hr),
                static_cast<int>(hr)
            );
        }

        static Error FromErrno(int err) {
            return Error(
                std::format("errno: {}", err),
                err
            );
        }

    private:
        std::string message_;
        std::optional<int> code_;
    };

}
