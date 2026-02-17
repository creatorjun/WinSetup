// src/adapters/persistence/config/IniConfigRepository.cpp
#include <adapters/persistence/config/IniConfigRepository.h>
#include <adapters/persistence/config/IniParser.h>
#include <algorithm>

namespace winsetup::adapters::persistence {

    domain::Expected<std::shared_ptr<domain::SetupConfig>>
        IniConfigRepository::LoadConfig(const std::wstring& filePath)
    {
        IniParser parser;
        auto parseResult = parser.Parse(filePath);
        if (!parseResult.HasValue())
            return parseResult.GetError();

        auto data = parseResult.Value();
        auto config = std::make_shared<domain::SetupConfig>();

        if (data.find(L"USERPROFILE") != data.end()) {
            auto result = ParseUserProfile(data[L"USERPROFILE"], config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (data.find(L"PARTITION") != data.end()) {
            auto result = ParsePartition(data[L"PARTITION"], config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (data.find(L"BACKUP") != data.end()) {
            auto result = ParseBackup(data[L"BACKUP"], config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (data.find(L"TYPES") != data.end()) {
            auto result = ParseTypes(data[L"TYPES"], config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (data.find(L"TIMES") != data.end()) {
            auto result = ParseTimes(data[L"TIMES"], config.get());
            if (!result.HasValue()) return result.GetError();
        }

        if (!config->IsValid())
            return domain::Error(L"Invalid configuration", 0, domain::ErrorCategory::Validation);

        return config;
    }

    domain::Expected<void>
        IniConfigRepository::SaveConfig(const std::wstring& filePath, const domain::SetupConfig& config)
    {
        return domain::Error(L"SaveConfig not implemented", 0, domain::ErrorCategory::NotImplemented);
    }

    domain::Expected<void>
        IniConfigRepository::ParseUserProfile(const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig* config)
    {
        auto it = section.find(L"USERPROFILE");
        if (it != section.end() && !it->second.empty())
            config->SetUserProfile(it->second);
        return domain::Expected<void>{};
    }

    domain::Expected<void>
        IniConfigRepository::ParsePartition(const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig* config)
    {
        auto it = section.find(L"DATAPARTITOM");
        if (it != section.end())
            config->SetDataPartition(ParseBool(it->second));
        return domain::Expected<void>{};
    }

    domain::Expected<void>
        IniConfigRepository::ParseBackup(const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig* config)
    {
        config->ClearBackupTargets();
        for (const auto& [key, value] : section)
            if (!key.empty() && !value.empty())
                config->AddBackupTarget(key, value);
        return domain::Expected<void>{};
    }

    domain::Expected<void>
        IniConfigRepository::ParseTypes(const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig* config)
    {
        config->ClearInstallationTypes();
        for (const auto& [key, value] : section)
            if (!key.empty() && !value.empty())
                config->AddInstallationType(key, value);
        return domain::Expected<void>{};
    }

    domain::Expected<void>
        IniConfigRepository::ParseTimes(const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig* config)
    {
        for (const auto& [key, value] : section) {
            if (key.empty() || value.empty()) continue;
            try {
                uint32_t seconds = static_cast<uint32_t>(std::stoul(value));
                config->SetEstimatedTime(key, seconds);
            }
            catch (...) { continue; }
        }
        return domain::Expected<void>{};
    }

    bool IniConfigRepository::ParseBool(const std::wstring& value) const
    {
        std::wstring upper = IniParser::ToUpper(value);
        return upper == L"TRUE" || upper == L"TURE" || upper == L"1" || upper == L"YES";
    }

}
