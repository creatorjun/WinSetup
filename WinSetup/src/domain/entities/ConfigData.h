#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <domain/primitives/Expected.h>

namespace winsetup::domain {

    struct SetupType {
        std::wstring name;
        std::wstring info;

        SetupType() = default;
        SetupType(std::wstring n, std::wstring i)
            : name(std::move(n)), info(std::move(i)) {
        }
    };

    struct BackupPath {
        std::wstring key;
        std::wstring path;

        BackupPath() = default;
        BackupPath(std::wstring k, std::wstring p)
            : key(std::move(k)), path(std::move(p)) {
        }

        [[nodiscard]] std::wstring ResolvePath(const std::wstring& userProfilePath) const {
            std::wstring resolved = path;
            size_t pos = resolved.find(L"{USERPROFILEPATH}");
            if (pos != std::wstring::npos) {
                resolved.replace(pos, 17, userProfilePath);
            }
            return resolved;
        }
    };

    class ConfigData {
    public:
        ConfigData() = default;

        [[nodiscard]] const std::wstring& GetUserProfile() const noexcept {
            return userProfile_;
        }

        void SetUserProfile(std::wstring profile) {
            userProfile_ = std::move(profile);
        }

        [[nodiscard]] const std::vector<BackupPath>& GetBackupPaths() const noexcept {
            return backupPaths_;
        }

        void AddBackupPath(BackupPath path) {
            backupPaths_.push_back(std::move(path));
        }

        [[nodiscard]] const std::vector<SetupType>& GetSetupTypes() const noexcept {
            return setupTypes_;
        }

        void AddSetupType(SetupType type) {
            setupTypes_.push_back(std::move(type));
        }

        [[nodiscard]] Expected<std::chrono::seconds> GetEstimatedTime(
            const std::wstring& hardwareModel
        ) const noexcept {
            auto it = estimatedTimes_.find(hardwareModel);
            if (it == estimatedTimes_.end()) {
                return Expected<std::chrono::seconds>::Failure(
                    Error("Hardware model not found in config")
                );
            }
            return Expected<std::chrono::seconds>::Success(it->second);
        }

        void SetEstimatedTime(std::wstring model, std::chrono::seconds time) {
            estimatedTimes_[std::move(model)] = time;
        }

        [[nodiscard]] const std::unordered_map<std::wstring, std::chrono::seconds>&
            GetAllEstimatedTimes() const noexcept {
            return estimatedTimes_;
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return !userProfile_.empty() && !setupTypes_.empty();
        }

        [[nodiscard]] size_t GetSetupTypeCount() const noexcept {
            return setupTypes_.size();
        }

        [[nodiscard]] size_t GetBackupPathCount() const noexcept {
            return backupPaths_.size();
        }

    private:
        std::wstring userProfile_;
        std::vector<BackupPath> backupPaths_;
        std::vector<SetupType> setupTypes_;
        std::unordered_map<std::wstring, std::chrono::seconds> estimatedTimes_;
    };

}
