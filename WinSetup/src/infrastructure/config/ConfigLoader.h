#pragma once

#include <memory>
#include "../../domain/entities/ConfigData.h"
#include "../../domain/primitives/Expected.h"
#include "IniParser.h"

namespace winsetup::infrastructure {

    class ConfigLoader {
    public:
        ConfigLoader();
        ~ConfigLoader() = default;

        ConfigLoader(const ConfigLoader&) = delete;
        ConfigLoader& operator=(const ConfigLoader&) = delete;
        ConfigLoader(ConfigLoader&&) noexcept = default;
        ConfigLoader& operator=(ConfigLoader&&) noexcept = default;

        [[nodiscard]] domain::Expected<domain::ConfigData>
            LoadFromFile(const std::wstring& filePath) const noexcept;

        [[nodiscard]] domain::Expected<domain::ConfigData>
            LoadFromContent(const std::wstring& content) const noexcept;

    private:
        [[nodiscard]] domain::Expected<domain::ConfigData>
            ParseSections(
                const std::unordered_map<std::wstring, IniSection>& sections
            ) const noexcept;

        void ParseUserProfile(
            domain::ConfigData& config,
            const IniSection& section
        ) const noexcept;

        void ParseBackups(
            domain::ConfigData& config,
            const IniSection& section
        ) const noexcept;

        void ParseTypes(
            domain::ConfigData& config,
            const IniSection& section
        ) const noexcept;

        void ParseTimes(
            domain::ConfigData& config,
            const IniSection& section
        ) const noexcept;

        [[nodiscard]] std::chrono::seconds ParseTimeValue(
            const std::wstring& value
        ) const noexcept;

        IniParser parser_;
    };

}
