// src/domain/events/DiskAnalyzedEvent.h

#pragma once

#include <domain/events/DomainEvent.h>
#include <domain/entities/DiskInfo.h>
#include <vector>

namespace winsetup::domain {

    class DiskAnalyzedEvent : public DomainEvent {
    public:
        explicit DiskAnalyzedEvent(std::vector<DiskInfo> disks)
            : m_disks(std::move(disks))
        {
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return L"DiskAnalyzed";
        }

        [[nodiscard]] const std::vector<DiskInfo>& GetDisks() const noexcept {
            return m_disks;
        }

    private:
        std::vector<DiskInfo> m_disks;
    };

}
