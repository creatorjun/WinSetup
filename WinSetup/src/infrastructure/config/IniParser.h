#pragma once

#include <string>
#include <vector>
#include <cwctype>
#include <unordered_map>
#include <domain/primitives/Expected.h>
#include <domain/primitives/Error.h>

namespace winsetup::infrastructure {

    struct IniSection {
        std::wstring name;
        std::unordered_map<std::wstring, std::wstring> values;
    };

    class IniParser {
    public:
        IniParser() = default;
        ~IniParser() = default;

        IniParser(const IniParser&) = delete;
        IniParser& operator=(const IniParser&) = delete;
        IniParser(IniParser&&) noexcept = default;
        IniParser& operator=(IniParser&&) noexcept = default;

        [[nodiscard]] domain::Expected<std::unordered_map<std::wstring, IniSection>>
            ParseFile(const std::wstring& filePath) const noexcept;

        [[nodiscard]] domain::Expected<std::unordered_map<std::wstring, IniSection>>
            ParseContent(const std::wstring& content) const noexcept;

    private:
        [[nodiscard]] std::wstring Trim(const std::wstring& str) const noexcept;

        [[nodiscard]] bool IsComment(const std::wstring& line) const noexcept;

        [[nodiscard]] bool IsSection(const std::wstring& line) const noexcept;

        [[nodiscard]] std::wstring ExtractSectionName(const std::wstring& line) const noexcept;

        [[nodiscard]] bool ParseKeyValue(
            const std::wstring& line,
            std::wstring& outKey,
            std::wstring& outValue
        ) const noexcept;
    };

}
