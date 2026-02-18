// src/domain/services/PathNormalizer.cpp
#include "PathNormalizer.h"
#include <algorithm>
#include <cwctype>

namespace winsetup::domain {

    std::wstring PathNormalizer::ResolvePath(
        const std::wstring& path,
        const std::wstring& userProfile
    ) const {
        std::wstring result = path;
        const std::wstring placeholder = L"{USERPROFILE}";
        size_t pos = result.find(placeholder);
        while (pos != std::wstring::npos) {
            result.replace(pos, placeholder.length(), userProfile);
            pos = result.find(placeholder, pos + userProfile.length());
        }
        return result;
    }

    std::wstring PathNormalizer::SanitizeFolderName(const std::wstring& name) {
        static constexpr wchar_t kInvalidChars[] = L"\\/:*?\"<>|";
        std::wstring result;
        result.reserve(name.size());

        for (wchar_t ch : name) {
            bool invalid = false;
            for (const wchar_t* p = kInvalidChars; *p != L'\0'; ++p) {
                if (ch == *p) {
                    invalid = true;
                    break;
                }
            }
            if (!invalid) {
                result += ch;
            }
        }

        while (!result.empty() && (result.back() == L' ' || result.back() == L'.')) {
            result.pop_back();
        }
        while (!result.empty() && result.front() == L' ') {
            result.erase(result.begin());
        }

        if (result.empty()) {
            result = L"Unknown";
        }

        return result;
    }

}
