// src/adapters/persistence/config/IniParser.cpp

#include <adapters/persistence/config/IniParser.h>
#include <domain/primitives/Error.h>
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>

namespace winsetup::adapters::persistence {

    domain::Expected<std::wstring> IniParser::ReadFileAsWString(const std::wstring& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
            return domain::Error(L"Failed to open config file: " + filePath, 1,
                domain::ErrorCategory::IO);

        std::vector<uint8_t> bytes(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();

        if (bytes.empty())
            return std::wstring{};

        // UTF-16 LE BOM: FF FE
        if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
            const wchar_t* data = reinterpret_cast<const wchar_t*>(bytes.data() + 2);
            size_t         len = (bytes.size() - 2) / sizeof(wchar_t);
            return std::wstring(data, len);
        }

        // UTF-16 BE BOM: FE FF → swap to LE
        if (bytes.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
            for (size_t i = 2; i + 1 < bytes.size(); i += 2)
                std::swap(bytes[i], bytes[i + 1]);
            const wchar_t* data = reinterpret_cast<const wchar_t*>(bytes.data() + 2);
            size_t         len = (bytes.size() - 2) / sizeof(wchar_t);
            return std::wstring(data, len);
        }

        const char* src = reinterpret_cast<const char*>(bytes.data());
        int         srcSize = static_cast<int>(bytes.size());

        // UTF-8 BOM: EF BB BF
        if (bytes.size() >= 3 &&
            bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
            src += 3;
            srcSize -= 3;
        }

        // UTF-8 → wstring
        int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            src, srcSize, nullptr, 0);
        if (wlen > 0) {
            std::wstring result(wlen, L'\0');
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                src, srcSize, &result[0], wlen);
            return result;
        }

        // Fallback: ANSI/CP949
        wlen = MultiByteToWideChar(CP_ACP, 0, src, srcSize, nullptr, 0);
        if (wlen > 0) {
            std::wstring result(wlen, L'\0');
            MultiByteToWideChar(CP_ACP, 0, src, srcSize, &result[0], wlen);
            return result;
        }

        return domain::Error(L"Failed to decode config file: " + filePath, 2,
            domain::ErrorCategory::IO);
    }

    IniParser::Section* IniParser::FindOrCreateSection(IniData& data, const std::wstring& name) {
        for (auto& entry : data) {
            if (entry.first == name)
                return &entry.second;
        }
        data.emplace_back(name, Section{});
        return &data.back().second;
    }

    const IniParser::Section* IniParser::FindSection(const IniData& data, const std::wstring& name) {
        for (const auto& entry : data) {
            if (entry.first == name)
                return &entry.second;
        }
        return nullptr;
    }

    const std::wstring* IniParser::FindValue(const Section& section, const std::wstring& key) {
        for (const auto& kv : section) {
            if (kv.first == key)
                return &kv.second;
        }
        return nullptr;
    }

    domain::Expected<IniParser::IniData> IniParser::Parse(const std::wstring& filePath) {
        auto readResult = ReadFileAsWString(filePath);
        if (!readResult.HasValue())
            return readResult.GetError();
        return ParseContent(readResult.Value());
    }

    domain::Expected<IniParser::IniData> IniParser::ParseContent(const std::wstring& content) {
        IniData      data;
        std::wstring currentSection;
        Section* currentPtr = nullptr;

        std::wistringstream stream(content);
        std::wstring line;
        int lineNumber = 0;

        while (std::getline(stream, line)) {
            ++lineNumber;

            if (!line.empty() && line.back() == L'\r')
                line.pop_back();

            line = Trim(line);
            if (line.empty() || IsComment(line))
                continue;

            if (IsSection(line)) {
                currentSection = ExtractSectionName(line);
                if (currentSection.empty())
                    return domain::Error(
                        L"Invalid section name at line " + std::to_wstring(lineNumber),
                        lineNumber, domain::ErrorCategory::Parsing);
                currentPtr = FindOrCreateSection(data, currentSection);
                continue;
            }

            if (!currentPtr)
                continue;

            std::wstring key, value;
            if (ParseKeyValue(line, key, value))
                currentPtr->emplace_back(std::move(key), std::move(value));
        }

        return data;
    }

    std::wstring IniParser::Trim(const std::wstring& str) {
        auto start = std::find_if_not(str.begin(), str.end(), [](wchar_t c) { return std::iswspace(c); });
        auto end = std::find_if_not(str.rbegin(), str.rend(), [](wchar_t c) { return std::iswspace(c); }).base();
        return (start < end) ? std::wstring(start, end) : std::wstring{};
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
        return !line.empty() && line.front() == L'[' && line.back() == L']';
    }

    std::wstring IniParser::ExtractSectionName(const std::wstring& line) const {
        if (line.length() < 3) return L"";
        return Trim(line.substr(1, line.length() - 2));
    }

    bool IniParser::ParseKeyValue(const std::wstring& line,
        std::wstring& key,
        std::wstring& value) const {
        size_t pos = line.find(L'=');
        if (pos == std::wstring::npos) return false;
        key = Trim(line.substr(0, pos));
        value = Trim(line.substr(pos + 1));
        return !key.empty();
    }

}
