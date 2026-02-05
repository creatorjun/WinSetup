#pragma once

#include <string>
#include <string_view>
#include "../../domain/primitives/Error.h"
#include "../../domain/primitives/Expected.h"

namespace winsetup::abstractions {

    enum class TextEncoding {
        UTF8,
        UTF16LE,
        UTF16BE,
        ANSI,
        ASCII
    };

    class ITextEncoder {
    public:
        virtual ~ITextEncoder() = default;

        virtual domain::Expected<std::wstring> ToWide(
            std::string_view utf8
        ) const noexcept = 0;

        virtual domain::Expected<std::string> ToUtf8(
            std::wstring_view wide
        ) const noexcept = 0;

        virtual domain::Expected<std::wstring> Decode(
            std::string_view input,
            TextEncoding encoding
        ) const noexcept = 0;

        virtual domain::Expected<std::string> Encode(
            std::wstring_view input,
            TextEncoding encoding
        ) const noexcept = 0;

        virtual domain::Expected<size_t> GetEncodedSize(
            std::string_view input,
            TextEncoding fromEncoding,
            TextEncoding toEncoding
        ) const noexcept = 0;

        virtual bool IsValidUtf8(std::string_view input) const noexcept = 0;
        virtual bool IsValidUtf16(std::wstring_view input) const noexcept = 0;
    };

}
