// src/domain/entities/SetupConfig.h
#pragma once

#include <string>
#include <vector>

namespace winsetup::domain {

    class SetupConfig {
    public:
        SetupConfig() = default;

        [[nodiscard]] const std::wstring& GetWimPath() const noexcept { return m_wimPath; }
        [[nodiscard]] int GetWimIndex() const noexcept { return m_wimIndex; }
        [[nodiscard]] uint32_t GetTargetDiskIndex() const noexcept { return m_targetDiskIndex; }
        [[nodiscard]] const std::wstring& GetComputerName() const noexcept { return m_computerName; }
        [[nodiscard]] bool GetAutoReboot() const noexcept { return m_autoReboot; }
        [[nodiscard]] const std::vector<std::wstring>& GetDriverPaths() const noexcept { return m_driverPaths; }

        void SetWimPath(const std::wstring& path) { m_wimPath = path; }
        void SetWimPath(std::wstring&& path) noexcept { m_wimPath = std::move(path); }
        void SetWimIndex(int index) noexcept { m_wimIndex = index; }
        void SetTargetDiskIndex(uint32_t index) noexcept { m_targetDiskIndex = index; }
        void SetComputerName(const std::wstring& name) { m_computerName = name; }
        void SetComputerName(std::wstring&& name) noexcept { m_computerName = std::move(name); }
        void SetAutoReboot(bool reboot) noexcept { m_autoReboot = reboot; }
        void AddDriverPath(const std::wstring& path) { m_driverPaths.push_back(path); }

        [[nodiscard]] bool IsValid() const noexcept {
            return !m_wimPath.empty() && m_wimIndex >= 0;
        }

    private:
        std::wstring m_wimPath;
        int m_wimIndex = -1;
        uint32_t m_targetDiskIndex = 0;
        std::wstring m_computerName;
        bool m_autoReboot = false;
        std::vector<std::wstring> m_driverPaths;
    };

}
