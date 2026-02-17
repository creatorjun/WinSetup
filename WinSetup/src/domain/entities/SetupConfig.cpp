// src/domain/entities/SetupConfig.cpp
#include "SetupConfig.h"
#include <algorithm>

namespace winsetup::domain {

    SetupConfig::SetupConfig()
        : mUserProfile(L"User")
        , mHasDataPartition(false)
    {
    }

    void SetupConfig::AddBackupTarget(const std::wstring& name, const std::wstring& path) {
        mBackupTargets.emplace_back(name, path);
    }

    void SetupConfig::AddInstallationType(const std::wstring& name, const std::wstring& description) {
        mInstallationTypes.emplace_back(name, description);
    }

    bool SetupConfig::HasEstimatedTime(const std::wstring& motherboardModel) const {
        return mEstimatedTimes.find(motherboardModel) != mEstimatedTimes.end();
    }

    uint32_t SetupConfig::GetEstimatedTime(const std::wstring& motherboardModel) const {
        auto it = mEstimatedTimes.find(motherboardModel);
        if (it != mEstimatedTimes.end()) {
            return it->second;
        }
        return 180;
    }

    void SetupConfig::SetEstimatedTime(const std::wstring& motherboardModel, uint32_t seconds) {
        mEstimatedTimes[motherboardModel] = seconds;
    }

    std::wstring SetupConfig::ResolveBackupPath(const std::wstring& path) const {
        std::wstring resolved = path;

        size_t pos = resolved.find(L"{USERPROFILE}");
        if (pos != std::wstring::npos) {
            if (mHasDataPartition) {
                resolved.replace(pos, 13, L"D:\\" + mUserProfile);
            }
            else {
                resolved.replace(pos, 13, L"C:\\Users\\" + mUserProfile);
            }
        }

        std::replace(resolved.begin(), resolved.end(), L'/', L'\\');

        return resolved;
    }

    bool SetupConfig::IsValid() const noexcept {
        return !mUserProfile.empty();
    }

}
