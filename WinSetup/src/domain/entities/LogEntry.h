#pragma once

#include <string>
#include <chrono>
#include <abstractions/logging/ILogger.h>

namespace winsetup::domain {

    class LogEntry {
    public:
        LogEntry(
            abstractions::LogLevel level,
            std::wstring message,
            std::wstring category,
            uint32_t threadId
        )
            : level_(level)
            , message_(std::move(message))
            , category_(std::move(category))
            , timestamp_(std::chrono::system_clock::now())
            , threadId_(threadId) {
        }

        LogEntry(
            abstractions::LogLevel level,
            std::wstring message,
            std::wstring category = L""
        )
            : level_(level)
            , message_(std::move(message))
            , category_(std::move(category))
            , timestamp_(std::chrono::system_clock::now())
            , threadId_(0) {
        }

        [[nodiscard]] abstractions::LogLevel GetLevel() const noexcept {
            return level_;
        }

        [[nodiscard]] const std::wstring& GetLogMessage() const noexcept {
            return message_;
        }

        [[nodiscard]] const std::wstring& GetCategory() const noexcept {
            return category_;
        }

        [[nodiscard]] std::chrono::system_clock::time_point GetTimestamp() const noexcept {
            return timestamp_;
        }

        [[nodiscard]] uint32_t GetThreadId() const noexcept {
            return threadId_;
        }

        [[nodiscard]] bool HasCategory() const noexcept {
            return !category_.empty();
        }

        void SetThreadId(uint32_t threadId) noexcept {
            threadId_ = threadId;
        }

    private:
        abstractions::LogLevel level_;
        std::wstring message_;
        std::wstring category_;
        std::chrono::system_clock::time_point timestamp_;
        uint32_t threadId_;
    };

}
