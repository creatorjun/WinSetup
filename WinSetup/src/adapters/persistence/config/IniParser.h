// src/adapters/persistence/config/IniParser.h
#pragma once

#include <domain/primitives/Expected.h>
#include <string>
#include <map>
#include <vector>

namespace winsetup::adapters::persistence {

    class IniParser {
    public:
        using Section = std::map<std::wstring, std::wstring>;
        using IniData = std::map<std::wstring, Section>;

        IniParser() = default;
        ~IniParser() = default;

        [[nodiscard]] domain::Expected<IniData> Parse(const std::wstring& filePath);
        [[nodiscard]] domain::Expected<IniData> ParseContent(const std::wstring& content);

        [[nodiscard]] static std::wstring Trim(const std::wstring& str);
        [[nodiscard]] static std::wstring ToUpper(const std::wstring& str);

    private:
        [[nodiscard]] bool IsComment(const std::wstring& line) const;
        [[nodiscard]] bool IsSection(const std::wstring& line) const;
        [[nodiscard]] std::wstring ExtractSectionName(const std::wstring& line) const;
        [[nodiscard]] bool ParseKeyValue(const std::wstring& line, std::wstring& key, std::wstring& value) const;
    };

}
