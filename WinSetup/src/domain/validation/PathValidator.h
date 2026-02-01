// src/domain/validation/PathValidator.h

#pragma once

#include "ValidationRules.h"
#include "ValidationResult.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace winsetup::domain {

    class PathValidator {
    public:
        PathValidator() = default;

        ValidationResult Validate(const std::wstring& path) const {
            ValidationResult result = ValidationResult::Valid();

            for (const auto& rule : rules_) {
                if (!rule->Validate(path)) {
                    result.AddError(rule->GetErrorMessage());
                }
            }

            return result;
        }

        void AddRule(std::unique_ptr<IValidationRule> rule) {
            rules_.push_back(std::move(rule));
        }

        void RemoveRule(const std::string& ruleName) {
            rules_.erase(
                std::remove_if(rules_.begin(), rules_.end(),
                    [&ruleName](const auto& rule) {
                        return rule->GetName() == ruleName;
                    }),
                rules_.end()
            );
        }

        void ClearRules() {
            rules_.clear();
        }

        size_t RuleCount() const noexcept {
            return rules_.size();
        }

    private:
        std::vector<std::unique_ptr<IValidationRule>> rules_;
    };

}
