#pragma once

#include <string>
#include <algorithm>
#include <cwctype>
#include <unordered_set>

namespace winsetup::domain {

    class IValidationRule {
    public:
        virtual ~IValidationRule() = default;
        virtual bool Validate(const std::wstring& path) const = 0;
        virtual std::wstring GetErrorMessage() const = 0;
        virtual std::string GetName() const = 0;
    };

    class InvalidCharacterRule : public IValidationRule {
    public:
        InvalidCharacterRule()
            : invalidChars_(L"<>:\"|?*") {
        }

        bool Validate(const std::wstring& path) const override {
            return path.find_first_of(invalidChars_) == std::wstring::npos;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path contains invalid characters: < > : \" | ? *";
        }

        std::string GetName() const override {
            return "InvalidCharacterRule";
        }

    private:
        const std::wstring invalidChars_;
    };

    class MaxLengthRule : public IValidationRule {
    public:
        explicit MaxLengthRule(size_t maxLength = 260)
            : maxLength_(maxLength) {
        }

        bool Validate(const std::wstring& path) const override {
            return path.length() <= maxLength_;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path exceeds maximum length of " + std::to_wstring(maxLength_);
        }

        std::string GetName() const override {
            return "MaxLengthRule";
        }

    private:
        size_t maxLength_;
    };

    class AbsolutePathRule : public IValidationRule {
    public:
        bool Validate(const std::wstring& path) const override {
            if (path.length() < 3) {
                return false;
            }

            return ::iswalpha(static_cast<wint_t>(path[0])) &&
                path[1] == L':' &&
                (path[2] == L'\\' || path[2] == L'/');
        }

        std::wstring GetErrorMessage() const override {
            return L"Path must be absolute (e.g., C:\\path)";
        }

        std::string GetName() const override {
            return "AbsolutePathRule";
        }
    };

    class ReservedNameRule : public IValidationRule {
    public:
        ReservedNameRule() {
            reservedNames_ = {
                L"CON", L"PRN", L"AUX", L"NUL",
                L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
                L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
            };
        }

        bool Validate(const std::wstring& path) const override {
            size_t lastSlash = path.find_last_of(L"\\/");
            std::wstring fileName = (lastSlash != std::wstring::npos)
                ? path.substr(lastSlash + 1)
                : path;

            size_t dotPos = fileName.find(L'.');
            std::wstring baseName = (dotPos != std::wstring::npos)
                ? fileName.substr(0, dotPos)
                : fileName;

            std::wstring upperName = baseName;
            std::transform(upperName.begin(), upperName.end(), upperName.begin(),
                [](wchar_t c) { return static_cast<wchar_t>(::towupper(static_cast<wint_t>(c))); });

            return reservedNames_.find(upperName) == reservedNames_.end();
        }

        std::wstring GetErrorMessage() const override {
            return L"Path contains reserved Windows name (CON, PRN, AUX, etc.)";
        }

        std::string GetName() const override {
            return "ReservedNameRule";
        }

    private:
        std::unordered_set<std::wstring> reservedNames_;
    };

    class TrailingSpaceRule : public IValidationRule {
    public:
        bool Validate(const std::wstring& path) const override {
            if (path.empty()) {
                return true;
            }

            size_t lastSlash = path.find_last_of(L"\\/");
            if (lastSlash == std::wstring::npos) {
                return !::iswspace(static_cast<wint_t>(path.back()));
            }

            for (size_t i = lastSlash + 1; i < path.length(); ++i) {
                size_t nextSlash = path.find_first_of(L"\\/", i);
                size_t end = (nextSlash != std::wstring::npos) ? nextSlash : path.length();

                if (end > i && ::iswspace(static_cast<wint_t>(path[end - 1]))) {
                    return false;
                }

                if (nextSlash == std::wstring::npos) {
                    break;
                }
                i = nextSlash;
            }

            return true;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path component ends with space";
        }

        std::string GetName() const override {
            return "TrailingSpaceRule";
        }
    };

    class EmptyComponentRule : public IValidationRule {
    public:
        bool Validate(const std::wstring& path) const override {
            return path.find(L"\\\\") == std::wstring::npos &&
                path.find(L"//") == std::wstring::npos;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path contains empty component";
        }

        std::string GetName() const override {
            return "EmptyComponentRule";
        }
    };

}
