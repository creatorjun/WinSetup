#pragma once

#include <cstdint>
#include <string>
#include "VolumeInfo.h"

namespace winsetup::domain {

    enum class PartitionType : uint8_t {
        Unknown,
        System,
        MSR,
        Basic,
        LDM
    };

    struct PartitionId {
        uint32_t diskIndex;
        uint32_t partitionNumber;

        [[nodiscard]] constexpr auto operator<=>(const PartitionId&) const = default;
    };

    class PartitionInfo {
    public:
        PartitionInfo() = default;

        PartitionInfo(
            PartitionId id,
            uint64_t startingOffset,
            uint64_t size,
            PartitionType type
        )
            : id_(id)
            , startingOffset_(startingOffset)
            , sizeInBytes_(size)
            , partitionType_(type) {
        }

        [[nodiscard]] PartitionId GetId() const noexcept {
            return id_;
        }

        [[nodiscard]] uint64_t GetStartingOffset() const noexcept {
            return startingOffset_;
        }

        [[nodiscard]] uint64_t GetSizeInBytes() const noexcept {
            return sizeInBytes_;
        }

        [[nodiscard]] uint64_t GetSizeInMB() const noexcept {
            return sizeInBytes_ / (1024ULL * 1024ULL);
        }

        [[nodiscard]] PartitionType GetPartitionType() const noexcept {
            return partitionType_;
        }

        [[nodiscard]] const std::optional<VolumeInfo>& GetVolume() const noexcept {
            return volume_;
        }

        void SetVolume(VolumeInfo volume) {
            volume_ = std::move(volume);
        }

        [[nodiscard]] bool HasVolume() const noexcept {
            return volume_.has_value();
        }

        [[nodiscard]] bool IsBootable() const noexcept {
            return isBootable_;
        }

        void SetBootable(bool bootable) noexcept {
            isBootable_ = bootable;
        }

    private:
        PartitionId id_;
        uint64_t startingOffset_{ 0 };
        uint64_t sizeInBytes_{ 0 };
        PartitionType partitionType_{ PartitionType::Unknown };
        std::optional<VolumeInfo> volume_;
        bool isBootable_{ false };
    };

}
