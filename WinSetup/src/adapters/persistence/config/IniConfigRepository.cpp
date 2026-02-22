// src/adapters/persistence/config/IniConfigRepository.cpp
#include <adapters/persistence/config/IniConfigRepository.h>
#include <adapters/persistence/config/IniParser.h>
#include <algorithm>

namespace winsetup::adapters::persistence {

    domain::Expected<std::shared_ptr<domain::SetupConfig>>
        IniConfigRepository::LoadConfig(const std::wstring& filePath) {
        IniParser parser;
        auto parseResult = parser.Parse(filePath);
        if (!parseResult.HasValue())
            return parseResult.GetError();

        const auto& data = parseResult.Value();
        auto config = std::make_shared<domain::SetupConfig>();

        if (const auto* sec = IniParser::FindSection(data, L"USERPROFILE")) {
            auto result = ParseUserProfile(*sec, config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (const auto* sec = IniParser::FindSection(data, L"PARTITION")) {
            auto result = ParsePartition(*sec, config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (const auto* sec = IniParser::FindSection(data, L"BACKUP")) {
            auto result = ParseBackup(*sec, config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (const auto* sec = IniParser::FindSection(data, L"TYPES")) {
            auto result = ParseTypes(*sec, config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (const auto* sec = IniParser::FindSection(data, L"TIMES")) {
            auto result = ParseTimes(*sec, config.get());
            if (!result.HasValue()) return result.GetError();
        }
        if (const auto* sec = IniParser::FindSection(data, L"BITLOCKER")) {
            auto result = ParseBitLocker(*sec, config.get());
            if (!result.HasValue()) return result.GetError();
        }

        if (!config->IsValid())
            return domain::Error(L"Invalid configuration", 0,
                domain::ErrorCategory::Validation);

        mConfig = config;
        return mConfig;
    }

    domain::Expected<void>
        IniConfigRepository::SaveConfig(const std::wstring& filePath,
            const domain::SetupConfig& config) {
        return domain::Error(L"SaveConfig not implemented", 0,
            domain::ErrorCategory::NotImplemented);
    }

    domain::Expected<std::shared_ptr<domain::SetupConfig>>
        IniConfigRepository::GetConfig() const {
        if (!mConfig)
            return domain::Error(L"Configuration not loaded yet. Call LoadConfig() first.",
                0, domain::ErrorCategory::System);
        return mConfig;
    }

    bool IniConfigRepository::IsLoaded() const noexcept {
        return mConfig != nullptr;
    }

    domain::Expected<void>
        IniConfigRepository::ParseUserProfile(const IniParser::Section& section,
            domain::SetupConfig* config) {
        if (const auto* v = IniParser::FindValue(section, L"USERPROFILE"))
            if (!v->empty()) config->SetUserProfile(*v);
        return domain::Expected<void>();
    }

    domain::Expected<void>
        IniConfigRepository::ParsePartition(const IniParser::Section& section,
            domain::SetupConfig* config) {
        if (const auto* v = IniParser::FindValue(section, L"DATAPARTITION"))
            config->SetDataPartition(ParseBool(*v));
        return domain::Expected<void>();
    }

    domain::Expected<void>
        IniConfigRepository::ParseBackup(const IniParser::Section& section,
            domain::SetupConfig* config) {
        config->ClearBackupTargets();
        for (const auto& [key, value] : section) {
            if (!key.empty() && !value.empty())
                config->AddBackupTarget(key, value);
        }
        return domain::Expected<void>();
    }

    domain::Expected<void>
        IniConfigRepository::ParseTypes(const IniParser::Section& section,
            domain::SetupConfig* config) {
        config->ClearInstallationTypes();
        for (const auto& [key, value] : section) {
            if (!key.empty() && !value.empty())
                config->AddInstallationType(key, value);
        }
        return domain::Expected<void>();
    }

    domain::Expected<void>
        IniConfigRepository::ParseTimes(const IniParser::Section& section,
            domain::SetupConfig* config) {
        for (const auto& [key, value] : section) {
            if (key.empty() || value.empty()) continue;
            try {
                uint32_t seconds = static_cast<uint32_t>(std::stoul(value));
                config->SetEstimatedTime(key, seconds);
            }
            catch (...) {
                continue;
            }
        }
        return domain::Expected<void>();
    }

    domain::Expected<void>
        IniConfigRepository::ParseBitLocker(const IniParser::Section& section,
            domain::SetupConfig* config) {
        if (const auto* v = IniParser::FindValue(section, L"PINNUMBER"))
            if (!v->empty()) config->SetBitLockerPin(*v);
        return domain::Expected<void>();
    }

    bool IniConfigRepository::ParseBool(const std::wstring& value) const {
        const std::wstring upper = IniParser::ToUpper(value);
        return upper == L"TRUE" || upper == L"TURE" || upper == L"1" || upper == L"YES";
    }

}
