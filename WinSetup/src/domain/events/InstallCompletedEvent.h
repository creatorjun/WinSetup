// src/domain/events/InstallCompletedEvent.h

#pragma once

#include <domain/events/DomainEvent.h>
#include <string>
#include <chrono>

namespace winsetup::domain {

    class InstallCompletedEvent : public DomainEvent {
    public:
        InstallCompletedEvent(bool success, std::wstring message, int elapsedSeconds)
            : m_success(success)
            , m_message(std::move(message))
            , m_elapsedSeconds(elapsedSeconds)
        {
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return L"InstallCompleted";
        }

        [[nodiscard]] bool IsSuccess() const noexcept { return m_success; }
        [[nodiscard]] const std::wstring& GetMessage() const noexcept { return m_message; }
        [[nodiscard]] int GetElapsedSeconds() const noexcept { return m_elapsedSeconds; }

    private:
        bool m_success;
        std::wstring m_message;
        int m_elapsedSeconds;
    };

}
