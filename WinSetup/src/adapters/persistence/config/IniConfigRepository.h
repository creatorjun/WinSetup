// src/adapters/persistence/config/IniConfigRepository.h

#pragma once

#include <abstractions/repositories/IConfigRepository.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <adapters/persistence/config/IniParser.h>
#include <memory>
#include <string>

namespace winsetup::adapters::persistence {

    class IniConfigRepository : public abstractions::IConfigRepository {
    public:
        IniConfigRepository() = default;
        ~IniConfigRepository() override = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SetupConfig>>
            LoadConfig(const std::wstring& filePath) override;

        [[nodiscard]] domain::Expected<void>
            SaveConfig(const std::wstring& filePath,
                const domain::SetupConfig& config) override;

    private:
        [[nodiscard]] domain::Expected<void>
            ParseUserProfile(const IniParser::Section& section, domain::SetupConfig* config);

        [[nodiscard]] domain::Expected<void>
            ParsePartition(const IniParser::Section& section, domain::SetupConfig* config);

        [[nodiscard]] domain::Expected<void>
            ParseBackup(const IniParser::Section& section, domain::SetupConfig* config);

        [[nodiscard]] domain::Expected<void>
            ParseTypes(const IniParser::Section& section, domain::SetupConfig* config);

        [[nodiscard]] domain::Expected<void>
            ParseTimes(const IniParser::Section& section, domain::SetupConfig* config);

        [[nodiscard]] domain::Expected<void>
            ParseBitLocker(const IniParser::Section& section, domain::SetupConfig* config);

        [[nodiscard]] bool ParseBool(const std::wstring& value) const;
    };

}

