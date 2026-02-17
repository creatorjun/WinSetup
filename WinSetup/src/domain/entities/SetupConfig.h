// src/domain/entities/SetupConfig.h
#pragma once

#include <string>
#include <vector>
#include <map>

namespace winsetup::domain {

    struct BackupTarget {
        std::wstring name;
        std::wstring path;

        BackupTarget() = default;
        BackupTarget(const std::wstring& n, const std::wstring& p)
            : name(n), path(p) {
        }
    };

    struct InstallationType {
        std::wstring name;
        std::wstring description;

        InstallationType() = default;
        InstallationType(const std::wstring& n, const std::wstring& d)
            : name(n), description(d) {
        }
    };

    class SetupConfig {
    public:
        SetupConfig();
        ~SetupConfig() = default;

        [[nodiscard]] const std::wstring& GetUserProfile() const noexcept { return mUserProfile; }
        void SetUserProfile(const std::wstring& profile) { mUserProfile = profile; }

        [[nodiscard]] bool HasDataPartition() const noexcept { return mHasDataPartition; }
        void SetDataPartition(bool hasPartition) noexcept { mHasDataPartition = hasPartition; }

        [[nodiscard]] const std::vector<BackupTarget>& GetBackupTargets() const noexcept { return mBackupTargets; }
        void AddBackupTarget(const std::wstring& name, const std::wstring& path);
        void ClearBackupTargets() { mBackupTargets.clear(); }

        [[nodiscard]] const std::vector<InstallationType>& GetInstallationTypes() const noexcept { return mInstallationTypes; }
        void AddInstallationType(const std::wstring& name, const std::wstring& description);
        void ClearInstallationTypes() { mInstallationTypes.clear(); }

        [[nodiscard]] bool HasEstimatedTime(const std::wstring& motherboardModel) const;
        [[nodiscard]] uint32_t GetEstimatedTime(const std::wstring& motherboardModel) const;
        void SetEstimatedTime(const std::wstring& motherboardModel, uint32_t seconds);

        [[nodiscard]] std::wstring ResolveBackupPath(const std::wstring& path) const;

        [[nodiscard]] bool IsValid() const noexcept;

    private:
        std::wstring mUserProfile;
        bool mHasDataPartition;
        std::vector<BackupTarget> mBackupTargets;
        std::vector<InstallationType> mInstallationTypes;
        std::map<std::wstring, uint32_t> mEstimatedTimes;
    };

}
