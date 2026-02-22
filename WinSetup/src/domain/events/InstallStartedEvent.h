// src/domain/events/InstallStartedEvent.h
#pragma once

#include <domain/events/DomainEvent.h>
#include <string>

namespace winsetup::domain {

    class InstallStartedEvent : public DomainEventBase<InstallStartedEvent> {
    public:
        InstallStartedEvent(int diskIndex, std::wstring imagePath)
            : mDiskIndex(diskIndex)
            , mImagePath(std::move(imagePath))
        {
        }

        [[nodiscard]] static std::wstring StaticEventType() noexcept {
            return L"InstallStarted";
        }

        [[nodiscard]] std::wstring ToString() const override {
            return L"InstallStarted: disk=" + std::to_wstring(mDiskIndex)
                + L" image=" + mImagePath;
        }

        [[nodiscard]] int                 GetDiskIndex()  const noexcept { return mDiskIndex; }
        [[nodiscard]] const std::wstring& GetImagePath()  const noexcept { return mImagePath; }

    private:
        int          mDiskIndex;
        std::wstring mImagePath;
    };

}
