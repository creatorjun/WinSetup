// src/adapters/persistence/config/IniParser.cpp
#include "IniParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>

namespace winsetup::adapters::persistence {

    domain::Expected<IniParser::IniData> IniParser::Parse(const std::wstring& filePath) {
        std::wifstream file(filePath);
        if (!file.is_open()) {
            return domain::Error(
                L"Failed to open config file: " + filePath,
                1,
                domain::ErrorCategory::IO
            );
        }

        std::wstringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return ParseContent(buffer.str());
    }

    domain::Expected<IniParser::IniData> IniParser::ParseContent(const std::wstring& content) {
        IniData data;
        std::wstring currentSection;

        std::wistringstream stream(content);
        std::wstring line;
        int lineNumber = 0;

        while (std::getline(stream, line)) {
            lineNumber++;
            line = Trim(line);

            if (line.empty() || IsComment(line)) {
                continue;
            }

            if (IsSection(line)) {
                currentSection = ExtractSectionName(line);
                if (currentSection.empty()) {
                    return domain::Error(
                        L"Invalid section name at line " + std::to_wstring(lineNumber),
                        lineNumber,
                        domain::ErrorCategory::Parsing
                    );
                }
                data[currentSection] = Section{};
                continue;
            }

            if (currentSection.empty()) {
                continue;
            }

            std::wstring key, value;
            if (ParseKeyValue(line, key, value)) {
                data[currentSection][key] = value;
            }
        }

        return data;
    }

    std::wstring IniParser::Trim(const std::wstring& str) {
        auto start = std::find_if_not(str.begin(), str.end(), [](wchar_t c) {
            return ::iswspace(c);
            });

        auto end = std::find_if_not(str.rbegin(), str.rend(), [](wchar_t c) {
            return ::iswspace(c);
            }).base();

        return (start < end) ? std::wstring(start, end) : std::wstring();
    }

    std::wstring IniParser::ToUpper(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towupper);
        return result;
    }

    bool IniParser::IsComment(const std::wstring& line) const {
        return !line.empty() && (line[0] == L';' || line[0] == L'#');
    }

    bool IniParser::IsSection(const std::wstring& line) const {
        return !line.empty() && line[0] == L'[' && line[line.length() - 1] == L']';
    }

    std::wstring IniParser::ExtractSectionName(const std::wstring& line) const {
        if (line.length() < 3) {
            return L"";
        }
        return Trim(line.substr(1, line.length() - 2));
    }

    bool IniParser::ParseKeyValue(const std::wstring& line, std::wstring& key, std::wstring& value) const {
        size_t pos = line.find(L'=');
        if (pos == std::wstring::npos) {
            return false;
        }

        key = Trim(line.substr(0, pos));
        value = Trim(line.substr(pos + 1));

        return !key.empty();
    }

}
