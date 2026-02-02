#pragma once

#include <vector>
#include <string>
#include <algorithm>

namespace winsetup::domain {

    class ValidationResult {
    public:
        ValidationResult() = default;

        static ValidationResult Valid() {
            return ValidationResult();
        }

        static ValidationResult Invalid(std::vector<std::wstring> errors) {
            ValidationResult result;
            result.errors_ = std::move(errors);
            return result;
        }

        static ValidationResult Invalid(std::wstring error) {
            ValidationResult result;
            result.errors_.push_back(std::move(error));
            return result;
        }

        bool IsValid() const noexcept {
            return errors_.empty();
        }

        bool HasErrors() const noexcept {
            return !errors_.empty();
        }

        const std::vector<std::wstring>& GetErrors() const noexcept {
            return errors_;
        }

        void AddError(std::wstring error) {
            errors_.push_back(std::move(error));
        }

        void AddErrors(const std::vector<std::wstring>& errors) {
            errors_.insert(errors_.end(), errors.begin(), errors.end());
        }

        void Clear() noexcept {
            errors_.clear();
        }

        size_t ErrorCount() const noexcept {
            return errors_.size();
        }

        std::wstring GetFirstError() const {
            return errors_.empty() ? L"" : errors_.front();
        }

        std::wstring GetCombinedErrors(const std::wstring& separator = L"\n") const {
            if (errors_.empty()) {
                return L"";
            }

            std::wstring result;
            for (size_t i = 0; i < errors_.size(); ++i) {
                result += errors_[i];
                if (i < errors_.size() - 1) {
                    result += separator;
                }
            }
            return result;
        }

        explicit operator bool() const noexcept {
            return IsValid();
        }

    private:
        std::vector<std::wstring> errors_;
    };

}
