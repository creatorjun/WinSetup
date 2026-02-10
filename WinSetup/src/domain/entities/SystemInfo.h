// src/domain/entities/SystemInfo.h
#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "DiskInfo.h"
#include "VolumeInfo.h"

namespace winsetup::domain {

    class SystemInfo {
    public:
        SystemInfo() = default;

        [[nodiscard]] const std::wstring& GetMotherboardModel() const noexcept {
            return mmotherboardModel;
        }

        [[nodiscard]] const std::wstring& GetBIOSVersion() const noexcept {
            return mbiosVersion;
        }

        [[nodiscard]] bool IsUEFIBoot() const noexcept {
            return misUEFIBoot;
        }

        [[nodiscard]] uint64_t GetTotalMemoryBytes() const noexcept {
            return mtotalMemoryBytes;
        }

        [[nodiscard]] const std::vector<DiskInfo>& GetDisks() const noexcept {
            return mdisks;
        }

        [[nodiscard]] const std::vector<VolumeInfo>& GetVolumes() const noexcept {
            return mvolumes;
        }

        void SetMotherboardModel(const std::wstring& model) {
            mmotherboardModel = model;
        }

        void SetMotherboardModel(std::wstring&& model) noexcept {
            mmotherboardModel = std::move(model);
        }

        void SetBIOSVersion(const std::wstring& version) {
            mbiosVersion = version;
        }

        void SetBIOSVersion(std::wstring&& version) noexcept {
            mbiosVersion = std::move(version);
        }

        void SetUEFIBoot(bool uefi) noexcept {
            misUEFIBoot = uefi;
        }

        void SetTotalMemoryBytes(uint64_t bytes) noexcept {
            mtotalMemoryBytes = bytes;
        }

        void SetDisks(const std::vector<DiskInfo>& disks) {
            mdisks = disks;
        }

        void SetDisks(std::vector<DiskInfo>&& disks) noexcept {
            mdisks = std::move(disks);
        }

        void SetVolumes(const std::vector<VolumeInfo>& volumes) {
            mvolumes = volumes;
        }

        void SetVolumes(std::vector<VolumeInfo>&& volumes) noexcept {
            mvolumes = std::move(volumes);
        }

        void AddDisk(const DiskInfo& disk) {
            mdisks.push_back(disk);
        }

        void AddDisk(DiskInfo&& disk) {
            mdisks.push_back(std::move(disk));
        }

        void AddVolume(const VolumeInfo& volume) {
            mvolumes.push_back(volume);
        }

        void AddVolume(VolumeInfo&& volume) {
            mvolumes.push_back(std::move(volume));
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return !mdisks.empty() || !mvolumes.empty();
        }

        [[nodiscard]] size_t GetDiskCount() const noexcept {
            return mdisks.size();
        }

        [[nodiscard]] size_t GetVolumeCount() const noexcept {
            return mvolumes.size();
        }

        [[nodiscard]] double GetTotalMemoryGB() const noexcept {
            return static_cast<double>(mtotalMemoryBytes) / (1024.0 * 1024.0 * 1024.0);
        }

        [[nodiscard]] const DiskInfo* FindDiskByIndex(uint32_t index) const noexcept {
            auto it = std::find_if(mdisks.begin(), mdisks.end(),
                [index](const DiskInfo& disk) { return disk.GetIndex() == index; });
            return it != mdisks.end() ? &(*it) : nullptr;
        }

        [[nodiscard]] const VolumeInfo* FindVolumeByLetter(const std::wstring& letter) const noexcept {
            auto it = std::find_if(mvolumes.begin(), mvolumes.end(),
                [&letter](const VolumeInfo& volume) { return volume.GetLetter() == letter; });
            return it != mvolumes.end() ? &(*it) : nullptr;
        }

        [[nodiscard]] std::vector<DiskInfo> GetPhysicalDisks() const {
            std::vector<DiskInfo> result;
            result.reserve(mdisks.size());
            for (const auto& disk : mdisks) {
                if (disk.IsPhysicalDisk()) {
                    result.push_back(disk);
                }
            }
            return result;
        }

        [[nodiscard]] std::vector<DiskInfo> GetSSDDisks() const {
            std::vector<DiskInfo> result;
            result.reserve(mdisks.size());
            for (const auto& disk : mdisks) {
                if (disk.IsSolidState()) {
                    result.push_back(disk);
                }
            }
            return result;
        }

        [[nodiscard]] std::vector<VolumeInfo> GetSystemVolumes() const {
            std::vector<VolumeInfo> result;
            for (const auto& volume : mvolumes) {
                if (volume.IsSystemVolume() || volume.IsBootVolume()) {
                    result.push_back(volume);
                }
            }
            return result;
        }

        [[nodiscard]] bool HasUEFI() const noexcept {
            return misUEFIBoot;
        }

        [[nodiscard]] bool HasEnoughMemory(uint64_t requiredBytes) const noexcept {
            return mtotalMemoryBytes >= requiredBytes;
        }

        [[nodiscard]] bool HasMinimumRequirements() const noexcept {
            constexpr uint64_t MIN_MEMORY_GB = 2ULL * 1024 * 1024 * 1024;
            return HasEnoughMemory(MIN_MEMORY_GB) && !mdisks.empty();
        }

        void Clear() noexcept {
            mmotherboardModel.clear();
            mbiosVersion.clear();
            misUEFIBoot = false;
            mtotalMemoryBytes = 0;
            mdisks.clear();
            mvolumes.clear();
        }

    private:
        std::wstring mmotherboardModel;
        std::wstring mbiosVersion;
        bool misUEFIBoot = false;
        uint64_t mtotalMemoryBytes = 0;
        std::vector<DiskInfo> mdisks;
        std::vector<VolumeInfo> mvolumes;
    };

}
