// src/domain/primitives/Error.h
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <chrono>

namespace winsetup::domain {

    enum class ErrorCategory {
        Unknown,
        System,
        Disk,
        Volume,
        Imaging,
        Configuration,
        Network,
        Permission
    };

    struct ErrorContext {
        std::wstring function;
        std::wstring file;
        int line;
        std::chrono::system_clock::time_point timestamp;
    };

    class Error {
    public:
        Error()
            : m_message(L"Unknown error")
            , m_code(0)
            , m_category(ErrorCategory::Unknown)
            , m_timestamp(std::chrono::system_clock::now())
        {
        }

        Error(
            std::wstring message,
            uint32_t code = 0,
            ErrorCategory category = ErrorCategory::Unknown
        )
            : m_message(std::move(message))
            , m_code(code)
            , m_category(category)
            , m_timestamp(std::chrono::system_clock::now())
        {
        }

        [[nodiscard]] const std::wstring& GetMessage() const noexcept {
            return m_message;
        }

        [[nodiscard]] uint32_t GetCode() const noexcept {
            return m_code;
        }

        [[nodiscard]] ErrorCategory GetCategory() const noexcept {
            return m_category;
        }

        [[nodiscard]] auto GetTimestamp() const noexcept {
            return m_timestamp;
        }

        void AddContext(const ErrorContext& context) {
            m_contexts.push_back(context);
        }

        [[nodiscard]] const std::vector<ErrorContext>& GetContexts() const noexcept {
            return m_contexts;
        }

        [[nodiscard]] std::wstring ToString() const;

    private:
        std::wstring m_message;
        uint32_t m_code;
        ErrorCategory m_category;
        std::chrono::system_clock::time_point m_timestamp;
        std::vector<ErrorContext> m_contexts;
    };

}
