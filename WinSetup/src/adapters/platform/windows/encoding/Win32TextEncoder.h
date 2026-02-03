#pragma once

#include "../../../../abstractions/platform/ITextEncoder.h"
#include <Windows.h>
#include <memory>

namespace winsetup::adapters {

    class Win32TextEncoder : public abstractions::ITextEncoder {
    public:
        Win32TextEncoder() noexcept = default;
        ~Win32TextEncoder() override = default;

        Win32TextEncoder(const Win32TextEncoder&) = delete;
        Win32TextEncoder& operator=(const Win32TextEncoder&) = delete;
        Win32TextEncoder(Win32TextEncoder&&) noexcept = default;
        Win32TextEncoder& operator=(Win32TextEncoder&&) noexcept = default;

        domain::Expected<std::wstring> ToWide(
            std::string_view utf8
        ) const noexcept override;

        domain::Expected<std::string> ToUtf8(
            std::wstring_view wide
        ) const noexcept override;

        domain::Expected<std::wstring> Decode(
            std::string_view input,
            abstractions::TextEncoding encoding
        ) const noexcept override;

        domain::Expected<std::string> Encode(
            std::wstring_view input,
            abstractions::TextEncoding encoding
        ) const noexcept override;

        domain::Expected<size_t> GetEncodedSize(
            std::string_view input,
            abstractions::TextEncoding fromEncoding,
            abstractions::TextEncoding toEncoding
        ) const noexcept override;

        bool IsValidUtf8(std::string_view input) const noexcept override;
        bool IsValidUtf16(std::wstring_view input) const noexcept override;

    private:
        static constexpr DWORD GetCodePage(abstractions::TextEncoding encoding) noexcept;
        static constexpr DWORD GetConversionFlags(abstractions::TextEncoding encoding) noexcept;

        domain::Expected<std::wstring> MultiByteToWideImpl(
            std::string_view input,
            UINT codePage,
            DWORD flags
        ) const noexcept;

        domain::Expected<std::string> WideToMultiByteImpl(
            std::wstring_view input,
            UINT codePage,
            DWORD flags
        ) const noexcept;

        domain::Error CreateErrorFromLastError(
            std::string_view operation
        ) const noexcept;
    };

    std::unique_ptr<abstractions::ITextEncoder> CreateTextEncoder();

}
