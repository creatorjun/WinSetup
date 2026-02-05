#include "Win32TextEncoder.h"
#include <algorithm>

namespace winsetup::adapters {

    constexpr UINT Win32TextEncoder::GetCodePage(
        abstractions::TextEncoding encoding
    ) noexcept {
        switch (encoding) {
        case abstractions::TextEncoding::UTF8:
            return CP_UTF8;
        case abstractions::TextEncoding::UTF16LE:
            return 1200;
        case abstractions::TextEncoding::UTF16BE:
            return 1201;
        case abstractions::TextEncoding::ANSI:
            return CP_ACP;
        case abstractions::TextEncoding::ASCII:
            return 20127;
        default:
            return CP_UTF8;
        }
    }

    constexpr DWORD Win32TextEncoder::GetConversionFlags(
        abstractions::TextEncoding encoding
    ) noexcept {
        switch (encoding) {
        case abstractions::TextEncoding::UTF8:
            return MB_ERR_INVALID_CHARS;
        case abstractions::TextEncoding::ANSI:
            return MB_PRECOMPOSED;
        case abstractions::TextEncoding::ASCII:
            return MB_ERR_INVALID_CHARS;
        default:
            return 0;
        }
    }

    template<typename StringView>
    domain::Expected<int> Win32TextEncoder::ValidateAndGetSize(StringView input) const noexcept {
        if (input.empty()) [[unlikely]] {
            return domain::Expected<int>::Success(0);
        }
        if (input.size() > static_cast<size_t>(INT_MAX)) [[unlikely]] {
            return domain::Expected<int>::Failure(
                domain::Error("Input too large for conversion", ERROR_INVALID_PARAMETER)
            );
        }
        return domain::Expected<int>::Success(static_cast<int>(input.size()));
    }

    domain::Expected<std::wstring> Win32TextEncoder::ToWide(
        std::string_view utf8
    ) const noexcept {
        return MultiByteToWideImpl(
            utf8,
            CP_UTF8,
            MB_ERR_INVALID_CHARS
        );
    }

    domain::Expected<std::string> Win32TextEncoder::ToUtf8(
        std::wstring_view wide
    ) const noexcept {
        return WideToMultiByteImpl(
            wide,
            CP_UTF8,
            WC_ERR_INVALID_CHARS
        );
    }

    domain::Expected<std::wstring> Win32TextEncoder::Decode(
        std::string_view input,
        abstractions::TextEncoding encoding
    ) const noexcept {
        if (encoding == abstractions::TextEncoding::UTF16LE ||
            encoding == abstractions::TextEncoding::UTF16BE) {

            if (input.size() % 2 != 0) [[unlikely]] {
                return domain::Expected<std::wstring>::Failure(
                    domain::Error("Invalid UTF-16 input: odd byte count", ERROR_INVALID_DATA)
                );
            }

            std::wstring result;
            result.reserve(input.size() / 2);

            const auto* bytes = reinterpret_cast<const uint8_t*>(input.data());
            const size_t wcharCount = input.size() / 2;

            if (encoding == abstractions::TextEncoding::UTF16LE) {
                for (size_t i = 0; i < wcharCount; ++i) {
                    wchar_t wch = static_cast<wchar_t>(
                        bytes[i * 2] | (bytes[i * 2 + 1] << 8)
                        );
                    result.push_back(wch);
                }
            }
            else {
                for (size_t i = 0; i < wcharCount; ++i) {
                    wchar_t wch = static_cast<wchar_t>(
                        (bytes[i * 2] << 8) | bytes[i * 2 + 1]
                        );
                    result.push_back(wch);
                }
            }

            return domain::Expected<std::wstring>::Success(std::move(result));
        }

        const UINT codePage = GetCodePage(encoding);
        const DWORD flags = GetConversionFlags(encoding);
        return MultiByteToWideImpl(input, codePage, flags);
    }

    domain::Expected<std::string> Win32TextEncoder::Encode(
        std::wstring_view input,
        abstractions::TextEncoding encoding
    ) const noexcept {
        if (encoding == abstractions::TextEncoding::UTF16LE ||
            encoding == abstractions::TextEncoding::UTF16BE) {

            std::string result;
            result.reserve(input.size() * 2);

            if (encoding == abstractions::TextEncoding::UTF16LE) {
                for (wchar_t wch : input) {
                    result.push_back(static_cast<char>(wch & 0xFF));
                    result.push_back(static_cast<char>((wch >> 8) & 0xFF));
                }
            }
            else {
                for (wchar_t wch : input) {
                    result.push_back(static_cast<char>((wch >> 8) & 0xFF));
                    result.push_back(static_cast<char>(wch & 0xFF));
                }
            }

            return domain::Expected<std::string>::Success(std::move(result));
        }

        const UINT codePage = GetCodePage(encoding);
        const DWORD flags = (encoding == abstractions::TextEncoding::UTF8)
            ? WC_ERR_INVALID_CHARS
            : 0;
        return WideToMultiByteImpl(input, codePage, flags);
    }

    domain::Expected<size_t> Win32TextEncoder::GetEncodedSize(
        std::string_view input,
        abstractions::TextEncoding fromEncoding,
        abstractions::TextEncoding toEncoding
    ) const noexcept {
        if (input.empty()) [[unlikely]] {
            return domain::Expected<size_t>::Success(0);
        }

        auto wideResult = Decode(input, fromEncoding);
        if (wideResult.HasError()) [[unlikely]] {
            return domain::Expected<size_t>::Failure(std::move(wideResult).GetError());
        }

        auto encodedResult = Encode(std::move(wideResult).Value(), toEncoding);
        if (encodedResult.HasError()) [[unlikely]] {
            return domain::Expected<size_t>::Failure(std::move(encodedResult).GetError());
        }

        return domain::Expected<size_t>::Success(std::move(encodedResult).Value().size());
    }

    bool Win32TextEncoder::IsValidUtf8(std::string_view input) const noexcept {
        if (input.empty()) [[unlikely]] {
            return true;
        }

        const int requiredSize = ::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            input.data(),
            static_cast<int>(input.size()),
            nullptr,
            0
        );

        return requiredSize > 0;
    }

    bool Win32TextEncoder::IsValidUtf16(std::wstring_view input) const noexcept {
        if (input.empty()) [[unlikely]] {
            return true;
        }

        const int requiredSize = ::WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            input.data(),
            static_cast<int>(input.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        return requiredSize > 0;
    }

    domain::Expected<std::wstring> Win32TextEncoder::MultiByteToWideImpl(
        std::string_view input,
        UINT codePage,
        DWORD flags
    ) const noexcept {
        auto sizeResult = ValidateAndGetSize(input);
        if (sizeResult.HasError()) [[unlikely]] {
            return domain::Expected<std::wstring>::Failure(std::move(sizeResult).GetError());
        }

        const int inputSize = sizeResult.Value();
        if (inputSize == 0) [[unlikely]] {
            return domain::Expected<std::wstring>::Success(std::wstring{});
        }

        const int requiredSize = ::MultiByteToWideChar(
            codePage,
            flags,
            input.data(),
            inputSize,
            nullptr,
            0
        );

        if (requiredSize <= 0) [[unlikely]] {
            return domain::Expected<std::wstring>::Failure(
                CreateErrorFromLastError("MultiByteToWideChar size calculation")
            );
        }

        std::wstring result(static_cast<size_t>(requiredSize), L'\0');

        const int convertedSize = ::MultiByteToWideChar(
            codePage,
            flags,
            input.data(),
            inputSize,
            result.data(),
            requiredSize
        );

        if (convertedSize <= 0) [[unlikely]] {
            return domain::Expected<std::wstring>::Failure(
                CreateErrorFromLastError("MultiByteToWideChar conversion")
            );
        }

        return domain::Expected<std::wstring>::Success(std::move(result));
    }

    domain::Expected<std::string> Win32TextEncoder::WideToMultiByteImpl(
        std::wstring_view input,
        UINT codePage,
        DWORD flags
    ) const noexcept {
        auto sizeResult = ValidateAndGetSize(input);
        if (sizeResult.HasError()) [[unlikely]] {
            return domain::Expected<std::string>::Failure(std::move(sizeResult).GetError());
        }

        const int inputSize = sizeResult.Value();
        if (inputSize == 0) [[unlikely]] {
            return domain::Expected<std::string>::Success(std::string{});
        }

        const int requiredSize = ::WideCharToMultiByte(
            codePage,
            flags,
            input.data(),
            inputSize,
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (requiredSize <= 0) [[unlikely]] {
            return domain::Expected<std::string>::Failure(
                CreateErrorFromLastError("WideCharToMultiByte size calculation")
            );
        }

        std::string result(static_cast<size_t>(requiredSize), '\0');

        const int convertedSize = ::WideCharToMultiByte(
            codePage,
            flags,
            input.data(),
            inputSize,
            result.data(),
            requiredSize,
            nullptr,
            nullptr
        );

        if (convertedSize <= 0) [[unlikely]] {
            return domain::Expected<std::string>::Failure(
                CreateErrorFromLastError("WideCharToMultiByte conversion")
            );
        }

        return domain::Expected<std::string>::Success(std::move(result));
    }

    domain::Error Win32TextEncoder::CreateErrorFromLastError(
        std::string_view operation
    ) const noexcept {
        const DWORD errorCode = ::GetLastError();

        std::string message = "Win32TextEncoder: ";
        message.append(operation);

        switch (errorCode) {
        case ERROR_INSUFFICIENT_BUFFER:
            message.append(" - Insufficient buffer");
            break;
        case ERROR_INVALID_FLAGS:
            message.append(" - Invalid flags");
            break;
        case ERROR_INVALID_PARAMETER:
            message.append(" - Invalid parameter");
            break;
        case ERROR_NO_UNICODE_TRANSLATION:
            message.append(" - No Unicode translation");
            break;
        default:
            message.append(" - Error code: ");
            message.append(std::to_string(errorCode));
            break;
        }

        return domain::Error(std::move(message), static_cast<int>(errorCode));
    }

    std::unique_ptr<abstractions::ITextEncoder> CreateTextEncoder() {
        return std::make_unique<Win32TextEncoder>();
    }

}
