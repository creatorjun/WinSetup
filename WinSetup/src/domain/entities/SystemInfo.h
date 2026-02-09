// src/domain/entities/SystemInfo.h
#pragma once

#include <string>
#include <vector>
#include "DiskInfo.h"
#include "VolumeInfo.h"

namespace winsetup::domain {

    class SystemInfo {
    public:
        SystemInfo() = default;

        [[nodiscard]] const std::wstring& GetMotherboardModel() const noexcept { return m_motherboardModel; }
        [[nodiscard]] const std::wstring& GetBIOSVersion() const noexcept { return m_biosVersion; }
        [[nodiscard]] bool IsUEFIBoot() const noexcept { return m_isUEFIBoot; }
        [[nodiscard]] uint64_t GetTotalMemoryBytes() const noexcept { return m_totalMemoryBytes; }
        [[nodiscard]] const std::vector<DiskInfo>& GetDisks() const noexcept { return m_disks; }
        [[nodiscard]] const std::vector<VolumeInfo>& GetVolumes() const noexcept { return m_volumes; }

        void SetMotherboardModel(const std::wstring& model) { m_motherboardModel = model; }
        void SetMotherboardModel(std::wstring&& model) noexcept { m_motherboardModel = std::move(model); }
        void SetBIOSVersion(const std::wstring& version) { m_biosVersion = version; }
        void SetBIOSVersion(std::wstring&& version) noexcept { m_biosVersion = std::move(version); }
        void SetUEFIBoot(bool isUEFI) noexcept { m_isUEFIBoot = isUEFI; }
        void SetTotalMemoryBytes(uint64_t bytes) noexcept { m_totalMemoryBytes = bytes; }
        void SetDisks(const std::vector<DiskInfo>& disks) { m_disks = disks; }
        void SetDisks(std::vector<DiskInfo>&& disks) noexcept { m_disks = std::move(disks); }
        void SetVolumes(const std::vector<VolumeInfo>& volumes) { m_volumes = volumes; }
        void SetVolumes(std::vector<VolumeInfo>&& volumes) noexcept { m_volumes = std::move(volumes); }

        [[nodiscard]] bool MeetsMinimumRequirements() const noexcept {
            return m_totalMemoryBytes >= (4ULL * 1024 * 1024 * 1024) && !m_disks.empty();
        }

        [[nodiscard]] double GetTotalMemoryGB() const noexcept {
            return static_cast<double>(m_totalMemoryBytes) / (1024.0 * 1024.0 * 1024.0);
        }

    private:
        std::wstring m_motherboardModel;
        std::wstring m_biosVersion;
        bool m_isUEFIBoot = false;
        uint64_t m_totalMemoryBytes = 0;
        std::vector<DiskInfo> m_disks;
        std::vector<VolumeInfo> m_volumes;
    };

}
