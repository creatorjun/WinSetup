// src/domain/events/InstallStartedEvent.h

#pragma once

#include <domain/events/DomainEvent.h>
#include <string>

namespace winsetup::domain {

    class InstallStartedEvent : public DomainEvent {
    public:
        explicit InstallStartedEvent(int diskIndex, std::wstring imagePath)
            : m_diskIndex(diskIndex)
            , m_imagePath(std::move(imagePath))
        {
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return L"InstallStarted";
        }

        [[nodiscard]] int GetDiskIndex() const noexcept { return m_diskIndex; }
        [[nodiscard]] const std::wstring& GetImagePath() const noexcept { return m_imagePath; }

    private:
        int m_diskIndex;
        std::wstring m_imagePath;
    };

}
