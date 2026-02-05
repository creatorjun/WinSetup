#include "IniParser.h"
#include <fstream>
#include <sstream>
#include <Windows.h>

namespace winsetup::infrastructure {

    domain::Expected<std::unordered_map<std::wstring, IniSection>>
        IniParser::ParseFile(const std::wstring& filePath) const noexcept {

        HANDLE hFile = CreateFileW(
            filePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                domain::Error("Failed to open config file", GetLastError())
            );
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize)) {
            CloseHandle(hFile);
            return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                domain::Error("Failed to get file size", GetLastError())
            );
        }

        if (fileSize.QuadPart > 10 * 1024 * 1024) {
            CloseHandle(hFile);
            return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                domain::Error("Config file too large (>10MB)")
            );
        }

        std::vector<char> buffer(static_cast<size_t>(fileSize.QuadPart) + 1);
        DWORD bytesRead = 0;

        if (!ReadFile(hFile, buffer.data(), static_cast<DWORD>(fileSize.QuadPart), &bytesRead, nullptr)) {
            CloseHandle(hFile);
            return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                domain::Error("Failed to read config file", GetLastError())
            );
        }

        CloseHandle(hFile);
        buffer[bytesRead] = '\0';

        int wideSize = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, nullptr, 0);
        if (wideSize == 0) {
            return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                domain::Error("Failed to convert file content to wide string", GetLastError())
            );
        }

        std::wstring content(wideSize, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, content.data(), wideSize);

        return ParseContent(content);
    }

    domain::Expected<std::unordered_map<std::wstring, IniSection>>
        IniParser::ParseContent(const std::wstring& content) const noexcept {

        std::unordered_map<std::wstring, IniSection> sections;
        std::wstring currentSection;

        std::wistringstream stream(content);
        std::wstring line;
        int lineNumber = 0;

        while (std::getline(stream, line)) {
            ++lineNumber;
            line = Trim(line);

            if (line.empty() || IsComment(line)) {
                continue;
            }

            if (IsSection(line)) {
                currentSection = ExtractSectionName(line);
                if (currentSection.empty()) {
                    return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                        domain::Error("Invalid section name at line " + std::to_string(lineNumber))
                    );
                }

                if (sections.find(currentSection) == sections.end()) {
                    IniSection section;
                    section.name = currentSection;
                    sections[currentSection] = std::move(section);
                }
                continue;
            }

            if (currentSection.empty()) {
                return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Failure(
                    domain::Error("Key-value pair outside of section at line " + std::to_string(lineNumber))
                );
            }

            std::wstring key, value;
            if (ParseKeyValue(line, key, value)) {
                sections[currentSection].values[key] = value;
            }
        }

        return domain::Expected<std::unordered_map<std::wstring, IniSection>>::Success(
            std::move(sections)
        );
    }

    std::wstring IniParser::Trim(const std::wstring& str) const noexcept {
        if (str.empty()) {
            return str;
        }

        size_t start = 0;
        while (start < str.length() && std::iswspace(str[start])) {
            ++start;
        }

        if (start == str.length()) {
            return L"";
        }

        size_t end = str.length() - 1;
        while (end > start && std::iswspace(str[end])) {
            --end;
        }

        return str.substr(start, end - start + 1);
    }

    bool IniParser::IsComment(const std::wstring& line) const noexcept {
        return !line.empty() && (line[0] == L';' || line[0] == L'#');
    }

    bool IniParser::IsSection(const std::wstring& line) const noexcept {
        return line.length() >= 3 && line[0] == L'[' && line.back() == L']';
    }

    std::wstring IniParser::ExtractSectionName(const std::wstring& line) const noexcept {
        if (line.length() < 3) {
            return L"";
        }
        return Trim(line.substr(1, line.length() - 2));
    }

    bool IniParser::ParseKeyValue(
        const std::wstring& line,
        std::wstring& outKey,
        std::wstring& outValue
    ) const noexcept {
        size_t equalPos = line.find(L'=');
        if (equalPos == std::wstring::npos || equalPos == 0) {
            return false;
        }

        outKey = Trim(line.substr(0, equalPos));
        outValue = Trim(line.substr(equalPos + 1));

        return !outKey.empty();
    }

}
