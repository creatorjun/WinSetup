#include "ConfigLoader.h"
#include <algorithm>
#include <cwctype>

namespace winsetup::infrastructure {

    ConfigLoader::ConfigLoader()
        : parser_() {
    }

    domain::Expected<domain::ConfigData>
        ConfigLoader::LoadFromFile(const std::wstring& filePath) const noexcept {

        auto sectionsResult = parser_.ParseFile(filePath);
        if (sectionsResult.HasError()) {
            return domain::Expected<domain::ConfigData>::Failure(
                std::move(sectionsResult).GetError()
            );
        }

        return ParseSections(sectionsResult.Value());
    }

    domain::Expected<domain::ConfigData>
        ConfigLoader::LoadFromContent(const std::wstring& content) const noexcept {

        auto sectionsResult = parser_.ParseContent(content);
        if (sectionsResult.HasError()) {
            return domain::Expected<domain::ConfigData>::Failure(
                std::move(sectionsResult).GetError()
            );
        }

        return ParseSections(sectionsResult.Value());
    }

    domain::Expected<domain::ConfigData>
        ConfigLoader::ParseSections(
            const std::unordered_map<std::wstring, IniSection>& sections
        ) const noexcept {

        domain::ConfigData config;

        auto userProfileIt = sections.find(L"USERPROFILE");
        if (userProfileIt != sections.end()) {
            ParseUserProfile(config, userProfileIt->second);
        }

        auto backupsIt = sections.find(L"BACKUPS");
        if (backupsIt != sections.end()) {
            ParseBackups(config, backupsIt->second);
        }

        auto typesIt = sections.find(L"TYPES");
        if (typesIt != sections.end()) {
            ParseTypes(config, typesIt->second);
        }

        auto timesIt = sections.find(L"TIMES");
        if (timesIt != sections.end()) {
            ParseTimes(config, timesIt->second);
        }

        if (!config.IsValid()) {
            return domain::Expected<domain::ConfigData>::Failure(
                domain::Error("Invalid config: missing required fields")
            );
        }

        return domain::Expected<domain::ConfigData>::Success(std::move(config));
    }

    void ConfigLoader::ParseUserProfile(
        domain::ConfigData& config,
        const IniSection& section
    ) const noexcept {
        auto it = section.values.find(L"USERPROFILE");
        if (it != section.values.end()) {
            config.SetUserProfile(it->second);
        }
    }

    void ConfigLoader::ParseBackups(
        domain::ConfigData& config,
        const IniSection& section
    ) const noexcept {
        for (const auto& [key, value] : section.values) {
            domain::BackupPath backupPath(key, value);
            config.AddBackupPath(std::move(backupPath));
        }
    }

    void ConfigLoader::ParseTypes(
        domain::ConfigData& config,
        const IniSection& section
    ) const noexcept {
        std::unordered_map<std::wstring, std::wstring> names;
        std::unordered_map<std::wstring, std::wstring> infos;

        for (const auto& [key, value] : section.values) {
            if (key.find(L"TYPE_NAME_") == 0) {
                std::wstring index = key.substr(10);
                names[index] = value;
            }
            else if (key.find(L"TYPE_INFO_") == 0) {
                std::wstring index = key.substr(10);
                infos[index] = value;
            }
        }

        std::vector<std::wstring> indices;
        for (const auto& [index, name] : names) {
            indices.push_back(index);
        }

        std::sort(indices.begin(), indices.end());

        for (const auto& index : indices) {
            auto nameIt = names.find(index);
            auto infoIt = infos.find(index);

            if (nameIt != names.end()) {
                std::wstring name = nameIt->second;
                std::wstring info = (infoIt != infos.end()) ? infoIt->second : L"";

                domain::SetupType setupType(std::move(name), std::move(info));
                config.AddSetupType(std::move(setupType));
            }
        }
    }

    void ConfigLoader::ParseTimes(
        domain::ConfigData& config,
        const IniSection& section
    ) const noexcept {
        for (const auto& [model, timeStr] : section.values) {
            std::chrono::seconds time = ParseTimeValue(timeStr);
            if (time.count() > 0) {
                config.SetEstimatedTime(model, time);
            }
        }
    }

    std::chrono::seconds ConfigLoader::ParseTimeValue(
        const std::wstring& value
    ) const noexcept {
        if (value.empty()) {
            return std::chrono::seconds(0);
        }

        int seconds = 0;
        try {
            seconds = std::stoi(value);
        }
        catch (...) {
            return std::chrono::seconds(0);
        }

        return std::chrono::seconds(seconds);
    }

}
