// src/domain/events/DomainEvent.h

#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace winsetup::domain {

    class DomainEvent {
    public:
        virtual ~DomainEvent() = default;

        [[nodiscard]] uint64_t GetEventId() const noexcept { return m_eventId; }
        [[nodiscard]] auto GetTimestamp() const noexcept { return m_timestamp; }
        [[nodiscard]] virtual std::wstring GetEventType() const noexcept = 0;

    protected:
        DomainEvent()
            : m_eventId(GenerateEventId())
            , m_timestamp(std::chrono::system_clock::now())
        {
        }

    private:
        static uint64_t GenerateEventId() noexcept {
            static uint64_t counter = 0;
            return ++counter;
        }

        uint64_t m_eventId;
        std::chrono::system_clock::time_point m_timestamp;
    };

}
