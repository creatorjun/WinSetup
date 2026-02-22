// src/domain/entities/SystemInfo.h
#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <optional>

namespace winsetup::domain {

    class SystemInfo {
    public:
        SystemInfo() = default;

        [[nodiscard]] const std::wstring& GetMotherboardModel() const noexcept { return mMotherboardModel; }
        [[nodiscard]] const std::wstring& GetBIOSVersion()      const noexcept { return mBIOSVersion; }
        [[nodiscard]] bool                IsUEFIBoot()          const noexcept { return mIsUEFIBoot; }
        [[nodiscard]] uint64_t            GetTotalMemoryBytes() const noexcept { return mTotalMemoryBytes; }

        void SetMotherboardModel(const std::wstring& model) { mMotherboardModel = model; }
        void SetMotherboardModel(std::wstring&& model) { mMotherboardModel = std::move(model); }
        void SetBIOSVersion(const std::wstring& version) { mBIOSVersion = version; }
        void SetBIOSVersion(std::wstring&& version) { mBIOSVersion = std::move(version); }
        void SetUEFIBoot(bool isUEFI) noexcept { mIsUEFIBoot = isUEFI; }
        void SetTotalMemoryBytes(uint64_t bytes) noexcept { mTotalMemoryBytes = bytes; }

        void SetInstallStart() noexcept { mInstallStartTime = std::chrono::system_clock::now(); }
        void SetInstallEnd()   noexcept { mInstallEndTime = std::chrono::system_clock::now(); }

        [[nodiscard]] std::optional<std::chrono::system_clock::time_point> GetInstallStartTime() const noexcept {
            return mInstallStartTime;
        }
        [[nodiscard]] std::optional<std::chrono::system_clock::time_point> GetInstallEndTime() const noexcept {
            return mInstallEndTime;
        }

    private:
        std::wstring mMotherboardModel;
        std::wstring mBIOSVersion;
        bool         mIsUEFIBoot = false;
        uint64_t     mTotalMemoryBytes = 0;

        std::optional<std::chrono::system_clock::time_point> mInstallStartTime;
        std::optional<std::chrono::system_clock::time_point> mInstallEndTime;
    };

}
