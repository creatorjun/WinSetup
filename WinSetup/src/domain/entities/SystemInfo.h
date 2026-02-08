// src/domain/entities/SystemInfo.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include "DiskInfo.h"

namespace winsetup::domain {

    class SystemInfo {
    public:
        SystemInfo() = default;

        [[nodiscard]] int GetSystemDiskIndex() const noexcept {
            return m_systemDiskIndex;
        }

        [[nodiscard]] const std::wstring& GetSystemDiskType() const noexcept {
            return m_systemDiskType;
        }

        [[nodiscard]] int GetDataDiskIndex() const noexcept {
            return m_dataDiskIndex;
        }

        [[nodiscard]] int GetSystemVolumeIndex() const noexcept {
            return m_systemVolumeIndex;
        }

        [[nodiscard]] int GetDataVolumeIndex() const noexcept {
            return m_dataVolumeIndex;
        }

        [[nodiscard]] int GetBootVolumeIndex() const noexcept {
            return m_bootVolumeIndex;
        }

        [[nodiscard]] int GetSystemVolumeCount() const noexcept {
            return m_systemVolumeCount;
        }

        [[nodiscard]] const std::wstring& GetMainboard() const noexcept {
            return m_mainboard;
        }

        [[nodiscard]] int GetEstimatedTimeSec() const noexcept {
            return m_estimatedTimeSec;
        }

        [[nodiscard]] const std::vector<DiskInfo>& GetDisks() const noexcept {
            return m_disks;
        }

        [[nodiscard]] int GetInstallType() const noexcept {
            return m_installType;
        }

        [[nodiscard]] bool GetDataSave() const noexcept {
            return m_dataSave;
        }

        [[nodiscard]] bool GetBitlocker() const noexcept {
            return m_bitlocker;
        }

        void SetSystemDiskIndex(int index) noexcept { m_systemDiskIndex = index; }
        void SetSystemDiskType(const std::wstring& type) { m_systemDiskType = type; }
        void SetSystemDiskType(std::wstring&& type) noexcept { m_systemDiskType = std::move(type); }
        void SetDataDiskIndex(int index) noexcept { m_dataDiskIndex = index; }
        void SetSystemVolumeIndex(int index) noexcept { m_systemVolumeIndex = index; }
        void SetDataVolumeIndex(int index) noexcept { m_dataVolumeIndex = index; }
        void SetBootVolumeIndex(int index) noexcept { m_bootVolumeIndex = index; }
        void SetSystemVolumeCount(int count) noexcept { m_systemVolumeCount = count; }
        void SetMainboard(const std::wstring& mainboard) { m_mainboard = mainboard; }
        void SetMainboard(std::wstring&& mainboard) noexcept { m_mainboard = std::move(mainboard); }

        void SetEstimatedTimeSec(int seconds) noexcept {
            m_estimatedTimeSec = seconds > 0 ? seconds : DEFAULT_ESTIMATED_TIME_SEC;
        }

        void SetDisks(const std::vector<DiskInfo>& disks) { m_disks = disks; }
        void SetDisks(std::vector<DiskInfo>&& disks) noexcept { m_disks = std::move(disks); }
        void SetInstallType(int type) noexcept { m_installType = type; }
        void SetDataSave(bool dataSave) noexcept { m_dataSave = dataSave; }
        void SetBitlocker(bool bitlocker) noexcept { m_bitlocker = bitlocker; }

        void ReserveDisks(size_t capacity) {
            m_disks.reserve(capacity);
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_systemDiskIndex != INVALID_DISK_INDEX;
        }

        [[nodiscard]] bool HasSystemDisk() const noexcept {
            return m_systemDiskIndex != INVALID_DISK_INDEX;
        }

        [[nodiscard]] bool HasDataDisk() const noexcept {
            return m_dataDiskIndex != INVALID_DISK_INDEX;
        }

        [[nodiscard]] bool HasBootVolume() const noexcept {
            return m_bootVolumeIndex != VolumeInfo::INVALID_INDEX;
        }

        [[nodiscard]] bool HasDisks() const noexcept {
            return !m_disks.empty();
        }

        [[nodiscard]] int GetEstimatedTimeMin() const noexcept {
            return (m_estimatedTimeSec + 59) / 60;
        }

        [[nodiscard]] std::optional<DiskInfo> FindDiskByIndex(int index) const {
            auto it = std::find_if(m_disks.begin(), m_disks.end(),
                [index](const DiskInfo& disk) {
                    return disk.GetIndex() == index;
                });

            if (it != m_disks.end()) {
                return *it;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<VolumeInfo> FindVolumeByIndex(int volumeIndex) const {
            for (const auto& disk : m_disks) {
                for (const auto& volume : disk.GetVolumes()) {
                    if (volume.GetIndex() == volumeIndex) {
                        return volume;
                    }
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<DiskInfo> GetSystemDisk() const {
            if (m_systemDiskIndex == INVALID_DISK_INDEX) {
                return std::nullopt;
            }
            return FindDiskByIndex(m_systemDiskIndex);
        }

        [[nodiscard]] std::optional<DiskInfo> GetDataDisk() const {
            if (m_dataDiskIndex == INVALID_DISK_INDEX) {
                return std::nullopt;
            }
            return FindDiskByIndex(m_dataDiskIndex);
        }

        [[nodiscard]] int GetDiskCount() const noexcept {
            return static_cast<int>(m_disks.size());
        }

        [[nodiscard]] int GetTotalVolumeCount() const noexcept {
            size_t count = 0;
            for (const auto& disk : m_disks) {
                count += disk.GetVolumeCount();
            }
            return static_cast<int>(count);
        }

        void ClearDisks() {
            m_disks.clear();
        }

        static constexpr int DEFAULT_ESTIMATED_TIME_SEC = 300;
        static constexpr int DEFAULT_INSTALL_TYPE = 0;
        static constexpr int INVALID_DISK_INDEX = -1;

    private:
        int m_systemDiskIndex = INVALID_DISK_INDEX;
        std::wstring m_systemDiskType;
        int m_dataDiskIndex = INVALID_DISK_INDEX;
        int m_systemVolumeIndex = VolumeInfo::INVALID_INDEX;
        int m_dataVolumeIndex = VolumeInfo::INVALID_INDEX;
        int m_bootVolumeIndex = VolumeInfo::INVALID_INDEX;
        int m_systemVolumeCount = 0;
        std::wstring m_mainboard;
        int m_estimatedTimeSec = DEFAULT_ESTIMATED_TIME_SEC;
        std::vector<DiskInfo> m_disks;
        int m_installType = DEFAULT_INSTALL_TYPE;
        bool m_dataSave = false;
        bool m_bitlocker = false;
    };

}
