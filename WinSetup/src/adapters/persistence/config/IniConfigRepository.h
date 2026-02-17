// src/adapters/persistence/config/IniConfigRepository.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <string>

namespace winsetup::adapters::persistence {

    class IniConfigRepository {
    public:
        IniConfigRepository();
        ~IniConfigRepository() = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SetupConfig>>
            LoadConfig(const std::wstring& filePath);

        [[nodiscard]] domain::Expected<void>
            SaveConfig(const std::wstring& filePath, const domain::SetupConfig& config);

    private:
        [[nodiscard]] domain::Expected<void> ParseUserProfile(
            const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig& config
        );

        [[nodiscard]] domain::Expected<void> ParsePartition(
            const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig& config
        );

        [[nodiscard]] domain::Expected<void> ParseBackup(
            const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig& config
        );

        [[nodiscard]] domain::Expected<void> ParseTypes(
            const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig& config
        );

        [[nodiscard]] domain::Expected<void> ParseTimes(
            const std::map<std::wstring, std::wstring>& section,
            domain::SetupConfig& config
        );

        [[nodiscard]] bool ParseBool(const std::wstring& value) const;
    };

}
