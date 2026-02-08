// src/domain/events/ErrorOccurredEvent.h

#pragma once

#include <domain/events/DomainEvent.h>
#include <domain/primitives/Error.h>

namespace winsetup::domain {

    class ErrorOccurredEvent : public DomainEvent {
    public:
        explicit ErrorOccurredEvent(Error error)
            : m_error(std::move(error))
        {
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return L"ErrorOccurred";
        }

        [[nodiscard]] const Error& GetError() const noexcept {
            return m_error;
        }

    private:
        Error m_error;
    };

}
