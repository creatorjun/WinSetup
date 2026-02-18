// src/domain/entities/SystemInfo.h
#pragma once
#include "DiskInfo.h"
#include "VolumeInfo.h"
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace winsetup::domain {

    class SystemInfo {
    public:
        SystemInfo() = default;

        [[nodiscard]] const std::wstring& GetMotherboardModel() const noexcept { return mmotherboardModel; }
        [[nodiscard]] const std::wstring& GetBIOSVersion()      const noexcept { return mbiosVersion; }
        [[nodiscard]] bool                IsUEFIBoot()           const noexcept { return misUEFIBoot; }
        [[nodiscard]] uint64_t            GetTotalMemoryBytes()  const noexcept { return mtotalMemoryBytes; }
        [[nodiscard]] const std::vector<DiskInfo>& GetDisks()   const noexcept { return mdisks; }
        [[nodiscard]] const std::vector<VolumeInfo>& GetVolumes() const noexcept { return mvolumes; }

        void SetMotherboardModel(std::wstring model) { mmotherboardModel = std::move(model); }
        void SetBIOSVersion(std::wstring version) { mbiosVersion = std::move(version); }
        void SetUEFIBoot(bool uefi) noexcept { misUEFIBoot = uefi; }
        void SetTotalMemoryBytes(uint64_t bytes) noexcept { mtotalMemoryBytes = bytes; }
        void SetDisks(std::vector<DiskInfo> disks) noexcept { mdisks = std::move(disks); }
        void SetVolumes(std::vector<VolumeInfo> vols) noexcept { mvolumes = std::move(vols); }
        void AddDisk(DiskInfo disk) { mdisks.push_back(std::move(disk)); }
        void AddVolume(VolumeInfo vol) { mvolumes.push_back(std::move(vol)); }

        void SetInstallStartTime(std::chrono::system_clock::time_point tp) noexcept {
            minstallStartTime = tp;
        }
        void SetInstallEndTime(std::chrono::system_clock::time_point tp) noexcept {
            minstallEndTime = tp;
        }
        void ClearInstallTime() noexcept {
            minstallStartTime = std::nullopt;
            minstallEndTime = std::nullopt;
        }

        [[nodiscard]] std::optional<std::chrono::system_clock::time_point>
            GetInstallStartTime() const noexcept { return minstallStartTime; }
        [[nodiscard]] std::optional<std::chrono::system_clock::time_point>
            GetInstallEndTime()   const noexcept { return minstallEndTime; }

        [[nodiscard]] bool   IsValid()          const noexcept { return !mdisks.empty() && !mvolumes.empty(); }
        [[nodiscard]] size_t GetDiskCount()     const noexcept { return mdisks.size(); }
        [[nodiscard]] size_t GetVolumeCount()   const noexcept { return mvolumes.size(); }
        [[nodiscard]] double GetTotalMemoryGB() const noexcept {
            return static_cast<double>(mtotalMemoryBytes) / (1024.0 * 1024.0 * 1024.0);
        }

        [[nodiscard]] const DiskInfo* FindDiskByIndex(uint32_t index) const noexcept {
            auto it = std::find_if(mdisks.begin(), mdisks.end(),
                [index](const DiskInfo& d) { return d.GetIndex() == index; });
            return it != mdisks.end() ? &(*it) : nullptr;
        }

        [[nodiscard]] const VolumeInfo* FindVolumeByLetter(const std::wstring& letter) const noexcept {
            auto it = std::find_if(mvolumes.begin(), mvolumes.end(),
                [&letter](const VolumeInfo& v) { return v.GetLetter() == letter; });
            return it != mvolumes.end() ? &(*it) : nullptr;
        }

        [[nodiscard]] std::vector<DiskInfo> GetSSDDisks() const {
            std::vector<DiskInfo> result;
            result.reserve(mdisks.size());
            for (const auto& d : mdisks)
                if (d.IsSolidState()) result.push_back(d);
            return result;
        }

        [[nodiscard]] std::vector<DiskInfo> GetPhysicalDisks() const {
            std::vector<DiskInfo> result;
            result.reserve(mdisks.size());
            for (const auto& d : mdisks)
                if (d.IsPhysicalDisk()) result.push_back(d);
            return result;
        }

        [[nodiscard]] std::vector<VolumeInfo> GetSystemVolumes() const {
            std::vector<VolumeInfo> result;
            for (const auto& v : mvolumes)
                if (v.IsSystemVolume() || v.IsBootVolume()) result.push_back(v);
            return result;
        }

        [[nodiscard]] bool HasEnoughMemory(uint64_t required) const noexcept {
            return mtotalMemoryBytes >= required;
        }
        [[nodiscard]] bool HasMinimumRequirements() const noexcept {
            constexpr uint64_t kMinMemory = 2ULL * 1024 * 1024 * 1024;
            return HasEnoughMemory(kMinMemory) && !mdisks.empty();
        }

        void Clear() noexcept {
            mmotherboardModel.clear();
            mbiosVersion.clear();
            misUEFIBoot = false;
            mtotalMemoryBytes = 0;
            mdisks.clear();
            mvolumes.clear();
            minstallStartTime = std::nullopt;
            minstallEndTime = std::nullopt;
        }

    private:
        std::wstring            mmotherboardModel;
        std::wstring            mbiosVersion;
        bool                    misUEFIBoot = false;
        uint64_t                mtotalMemoryBytes = 0;
        std::vector<DiskInfo>   mdisks;
        std::vector<VolumeInfo> mvolumes;

        std::optional<std::chrono::system_clock::time_point> minstallStartTime;
        std::optional<std::chrono::system_clock::time_point> minstallEndTime;
    };

}
