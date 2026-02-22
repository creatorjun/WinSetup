// src/domain/events/InstallCompletedEvent.h
#pragma once

#include <domain/events/DomainEvent.h>
#include <string>

namespace winsetup::domain {

    class InstallCompletedEvent : public DomainEventBase<InstallCompletedEvent> {
    public:
        InstallCompletedEvent(bool success, std::wstring message, int elapsedSeconds)
            : mSuccess(success)
            , mMessage(std::move(message))
            , mElapsedSeconds(elapsedSeconds)
        {
        }

        [[nodiscard]] static std::wstring StaticEventType() noexcept {
            return L"InstallCompleted";
        }

        [[nodiscard]] std::wstring ToString() const override {
            return L"InstallCompleted: " + mMessage
                + (mSuccess ? L" [Success]" : L" [Failed]");
        }

        [[nodiscard]] bool              IsSuccess()        const noexcept { return mSuccess; }
        [[nodiscard]] const std::wstring& GetMessage()     const noexcept { return mMessage; }
        [[nodiscard]] int               GetElapsedSeconds() const noexcept { return mElapsedSeconds; }

    private:
        bool         mSuccess;
        std::wstring mMessage;
        int          mElapsedSeconds;
    };

}
