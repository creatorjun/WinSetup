// src/domain/entities/SetupConfig.h

#pragma once

#include <string>
#include <vector>

namespace winsetup::domain {

    struct InstallType {
        int index = 0;
        std::wstring name;
        std::wstring description;
        int estimatedSeconds = 300;

        [[nodiscard]] bool IsValid() const noexcept {
            return index >= 0 && !name.empty();
        }
    };

    struct TimeInfo {
        std::wstring modelName;
        int estimatedSeconds = 0;

        [[nodiscard]] bool IsValid() const noexcept {
            return !modelName.empty() && estimatedSeconds > 0;
        }
    };

    class SetupConfig {
    public:
        SetupConfig() = default;

        void AddInstallType(const InstallType& type) {
            m_installTypes.push_back(type);
        }

        void AddTimeInfo(const TimeInfo& info) {
            m_timeInfos.push_back(info);
        }

        [[nodiscard]] const std::vector<InstallType>& GetInstallTypes() const noexcept {
            return m_installTypes;
        }

        [[nodiscard]] const std::vector<TimeInfo>& GetTimeInfos() const noexcept {
            return m_timeInfos;
        }

        [[nodiscard]] const InstallType* FindInstallTypeByIndex(int index) const {
            for (const auto& type : m_installTypes) {
                if (type.index == index) {
                    return &type;
                }
            }
            return nullptr;
        }

        [[nodiscard]] const TimeInfo* FindTimeInfo(const std::wstring& modelName) const {
            for (const auto& info : m_timeInfos) {
                if (info.modelName == modelName) {
                    return &info;
                }
            }
            return nullptr;
        }

        [[nodiscard]] int CalculateEstimatedTime(int installTypeIndex, const std::wstring& motherboardModel) const {
            const auto* installType = FindInstallTypeByIndex(installTypeIndex);
            if (!installType) {
                return 300;
            }

            const auto* timeInfo = FindTimeInfo(motherboardModel);
            if (timeInfo) {
                return timeInfo->estimatedSeconds;
            }

            return installType->estimatedSeconds;
        }

        void Clear() {
            m_installTypes.clear();
            m_timeInfos.clear();
        }

    private:
        std::vector<InstallType> m_installTypes;
        std::vector<TimeInfo> m_timeInfos;
    };

}
