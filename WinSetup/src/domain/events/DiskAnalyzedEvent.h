// src/domain/events/DiskAnalyzedEvent.h
#pragma once

#include <domain/events/DomainEvent.h>
#include <domain/entities/DiskInfo.h>
#include <vector>

namespace winsetup::domain {

    class DiskAnalyzedEvent : public DomainEventBase<DiskAnalyzedEvent> {
    public:
        explicit DiskAnalyzedEvent(std::vector<DiskInfo> disks)
            : mDisks(std::move(disks))
        {
        }

        [[nodiscard]] static std::wstring StaticEventType() noexcept {
            return L"DiskAnalyzed";
        }

        [[nodiscard]] std::wstring ToString() const override {
            return L"DiskAnalyzed: " + std::to_wstring(mDisks.size()) + L" disk(s)";
        }

        [[nodiscard]] const std::vector<DiskInfo>& GetDisks() const noexcept {
            return mDisks;
        }

    private:
        std::vector<DiskInfo> mDisks;
    };

}
