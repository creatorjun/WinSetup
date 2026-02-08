// src/domain/events/InstallProgressEvent.h

#pragma once

#include <domain/events/DomainEvent.h>
#include <string>

namespace winsetup::domain {

    class InstallProgressEvent : public DomainEvent {
    public:
        InstallProgressEvent(int percentage, std::wstring message, std::wstring stage)
            : m_percentage(percentage)
            , m_message(std::move(message))
            , m_stage(std::move(stage))
        {
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return L"InstallProgress";
        }

        [[nodiscard]] int GetPercentage() const noexcept { return m_percentage; }
        [[nodiscard]] const std::wstring& GetMessage() const noexcept { return m_message; }
        [[nodiscard]] const std::wstring& GetStage() const noexcept { return m_stage; }

    private:
        int m_percentage;
        std::wstring m_message;
        std::wstring m_stage;
    };

}
