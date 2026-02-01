// src/domain/validation/ValidationResult.h

#pragma once

#include <vector>
#include <string>
#include <algorithm>

namespace winsetup::domain {

    class ValidationResult {
    public:
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

        bool IsInvalid() const noexcept {
            return !errors_.empty();
        }

        const std::vector<std::wstring>& Errors() const noexcept {
            return errors_;
        }

        void AddError(std::wstring error) {
            errors_.push_back(std::move(error));
        }

        void Merge(const ValidationResult& other) {
            errors_.insert(errors_.end(), other.errors_.begin(), other.errors_.end());
        }

        size_t ErrorCount() const noexcept {
            return errors_.size();
        }

        void Clear() {
            errors_.clear();
        }

        explicit operator bool() const noexcept {
            return IsValid();
        }

    private:
        ValidationResult() = default;

        std::vector<std::wstring> errors_;
    };

}
