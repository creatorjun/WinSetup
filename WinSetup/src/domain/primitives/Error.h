// src/domain/primitives/Error.h
#pragma once

#include <string>
#include <cstdint>

namespace winsetup::domain {

    enum class ErrorCategory {
        Unknown = 0,
        System = 1,
        IO = 2,
        Parsing = 3,
        Validation = 4,
        NotImplemented = 5,
        Disk = 100,
        Volume = 101,
        Partition = 102,
        FileSystem = 103,
        Imaging = 200,
        Driver = 201,
        Network = 300,
        Registry = 301
    };

    class Error {
    public:
        Error();
        Error(const std::wstring& message, uint32_t code, ErrorCategory category);
        ~Error() = default;

        Error(const Error& other) = default;
        Error(Error&& other) noexcept = default;
        Error& operator=(const Error& other) = default;
        Error& operator=(Error&& other) noexcept = default;

        [[nodiscard]] const std::wstring& GetMessage() const noexcept { return mMessage; }
        [[nodiscard]] uint32_t GetCode() const noexcept { return mCode; }
        [[nodiscard]] ErrorCategory GetCategory() const noexcept { return mCategory; }

        [[nodiscard]] std::wstring ToString() const;

        [[nodiscard]] static std::wstring CategoryToString(ErrorCategory category);

    private:
        std::wstring mMessage;
        uint32_t mCode;
        ErrorCategory mCategory;
    };

}
