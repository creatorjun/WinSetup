// src/adapters/persistence/config/IniParser.h
#pragma once

#include <domain/primitives/Expected.h>
#include <string>
#include <vector>
#include <utility>

namespace winsetup::adapters::persistence {

    class IniParser {
    public:
        using KeyValue = std::pair<std::wstring, std::wstring>;
        using Section = std::vector<KeyValue>;
        using SectionEntry = std::pair<std::wstring, Section>;
        using IniData = std::vector<SectionEntry>;

        IniParser() = default;
        ~IniParser() = default;

        [[nodiscard]] domain::Expected<IniData> Parse(const std::wstring& filePath);
        [[nodiscard]] domain::Expected<IniData> ParseContent(const std::wstring& content);

        [[nodiscard]] static std::wstring          Trim(const std::wstring& str);
        [[nodiscard]] static std::wstring          ToUpper(const std::wstring& str);
        [[nodiscard]] static const Section* FindSection(const IniData& data, const std::wstring& name);
        [[nodiscard]] static const std::wstring* FindValue(const Section& section, const std::wstring& key);

    private:
        [[nodiscard]] static domain::Expected<std::wstring> ReadFileAsWString(const std::wstring& filePath);
        
        [[nodiscard]] bool IsComment(const std::wstring& line)  const;
        [[nodiscard]] bool IsSection(const std::wstring& line)  const;
        [[nodiscard]] std::wstring ExtractSectionName(const std::wstring& line) const;
        [[nodiscard]] bool ParseKeyValue(const std::wstring& line,
            std::wstring& key,
            std::wstring& value) const;
        static Section* FindOrCreateSection(IniData& data, const std::wstring& name,
            std::wstring& lastSectionName,
            Section*& lastSectionPtr);
    };

}
