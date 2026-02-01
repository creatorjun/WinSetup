// src/domain/validation/ValidationRules.h

#pragma once

#include <string>

namespace winsetup::domain {

    class IValidationRule {
    public:
        virtual ~IValidationRule() = default;
        virtual std::string GetName() const = 0;
        virtual bool Validate(const std::wstring& path) const = 0;
        virtual std::wstring GetErrorMessage() const = 0;
    };

    class InvalidCharacterRule : public IValidationRule {
    public:
        std::string GetName() const override {
            return "InvalidCharacterRule";
        }

        bool Validate(const std::wstring& path) const override {
            const std::wstring invalidChars = L"<>:\"|?*";
            return path.find_first_of(invalidChars) == std::wstring::npos;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path contains invalid characters: < > : \" | ? *";
        }
    };

    class MaxLengthRule : public IValidationRule {
    public:
        explicit MaxLengthRule(size_t maxLength) : maxLength_(maxLength) {}

        std::string GetName() const override {
            return "MaxLengthRule";
        }

        bool Validate(const std::wstring& path) const override {
            return path.length() <= maxLength_;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path exceeds maximum length of " + std::to_wstring(maxLength_);
        }

    private:
        size_t maxLength_;
    };

    class AbsolutePathRule : public IValidationRule {
    public:
        std::string GetName() const override {
            return "AbsolutePathRule";
        }

        bool Validate(const std::wstring& path) const override {
            if (path.length() < 3) return false;
            return (path[1] == L':' && path[2] == L'\\');
        }

        std::wstring GetErrorMessage() const override {
            return L"Path must be absolute (e.g., C:\\...)";
        }
    };

    class ReservedNameRule : public IValidationRule {
    public:
        std::string GetName() const override {
            return "ReservedNameRule";
        }

        bool Validate(const std::wstring& path) const override {
            const std::wstring reserved[] = {
                L"CON", L"PRN", L"AUX", L"NUL",
                L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
                L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
            };

            for (const auto& name : reserved) {
                if (path.find(name) != std::wstring::npos) {
                    return false;
                }
            }
            return true;
        }

        std::wstring GetErrorMessage() const override {
            return L"Path contains reserved device name";
        }
    };

}
