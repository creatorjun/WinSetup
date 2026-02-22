// src/domain/events/InstallProgressEvent.h
#pragma once

#include <domain/events/DomainEvent.h>
#include <string>

namespace winsetup::domain {

    class InstallProgressEvent : public DomainEventBase<InstallProgressEvent> {
    public:
        InstallProgressEvent(int percentage, std::wstring message, std::wstring stage)
            : mPercentage(percentage)
            , mMessage(std::move(message))
            , mStage(std::move(stage))
        {
        }

        [[nodiscard]] static std::wstring StaticEventType() noexcept {
            return L"InstallProgress";
        }

        [[nodiscard]] std::wstring ToString() const override {
            return L"InstallProgress: [" + mStage + L"] "
                + std::to_wstring(mPercentage) + L"% - " + mMessage;
        }

        [[nodiscard]] int                 GetPercentage() const noexcept { return mPercentage; }
        [[nodiscard]] const std::wstring& GetMessage()    const noexcept { return mMessage; }
        [[nodiscard]] const std::wstring& GetStage()      const noexcept { return mStage; }

    private:
        int          mPercentage;
        std::wstring mMessage;
        std::wstring mStage;
    };

}
