#pragma once

#include "ValidationRules.h"
#include "ValidationResult.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace winsetup::domain {

    enum class ValidationMode {
        AllErrors,
        FastFail
    };

    class PathValidator {
    public:
        PathValidator() = default;

        [[nodiscard]] ValidationResult Validate(
            const std::wstring& path,
            ValidationMode mode = ValidationMode::AllErrors
        ) const {
            ValidationResult result = ValidationResult::Valid();

            for (const auto& rule : rules_) {
                if (!rule->Validate(path)) [[unlikely]] {
                    result.AddError(rule->GetErrorMessage());
                    if (mode == ValidationMode::FastFail) [[unlikely]] {
                        break;
                    }
                }
            }

            return result;
        }

        [[nodiscard]] bool IsValid(const std::wstring& path) const noexcept {
            for (const auto& rule : rules_) {
                if (!rule->Validate(path)) [[unlikely]] {
                    return false;
                }
            }
            return true;
        }

        void AddRule(std::unique_ptr<IValidationRule> rule) {
            if (rule) [[likely]] {
                rules_.push_back(std::move(rule));
            }
        }

        void RemoveRule(const std::string& ruleName) {
            auto it = std::remove_if(rules_.begin(), rules_.end(),
                [&ruleName](const std::unique_ptr<IValidationRule>& rule) {
                    return rule->GetName() == ruleName;
                });
            rules_.erase(it, rules_.end());
        }

        void ClearRules() noexcept {
            rules_.clear();
        }

        [[nodiscard]] size_t RuleCount() const noexcept {
            return rules_.size();
        }

        [[nodiscard]] bool HasRule(const std::string& ruleName) const noexcept {
            return std::any_of(rules_.begin(), rules_.end(),
                [&ruleName](const std::unique_ptr<IValidationRule>& rule) {
                    return rule->GetName() == ruleName;
                });
        }

    private:
        std::vector<std::unique_ptr<IValidationRule>> rules_;
    };

}
