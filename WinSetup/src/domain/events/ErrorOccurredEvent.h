// src/domain/events/ErrorOccurredEvent.h
#pragma once

#include <domain/events/DomainEvent.h>
#include <domain/primitives/Error.h>

namespace winsetup::domain {

    class ErrorOccurredEvent : public DomainEventBase<ErrorOccurredEvent> {
    public:
        explicit ErrorOccurredEvent(Error error)
            : mError(std::move(error))
        {
        }

        [[nodiscard]] static std::wstring StaticEventType() noexcept {
            return L"ErrorOccurred";
        }

        [[nodiscard]] std::wstring ToString() const override {
            return L"ErrorOccurred: " + mError.GetMessage();
        }

        [[nodiscard]] const Error& GetError() const noexcept {
            return mError;
        }

    private:
        Error mError;
    };

}
