// src/domain/valueobjects/DriveLetter.cpp
#include "DriveLetter.h"
#include <cctype>

namespace winsetup::domain {

    Expected<DriveLetter> DriveLetter::Create(wchar_t letter) {
        wchar_t upper = static_cast<wchar_t>(::towupper(letter));

        if (upper < L'A' || upper > L'Z') {
            return Error{
                L"Invalid drive letter: must be A-Z",
                0,
                ErrorCategory::Unknown
            };
        }

        return DriveLetter(upper);
    }

    std::wstring DriveLetter::ToString() const {
        if (!IsValid()) return L"";
        return std::wstring(1, m_letter) + L":";
    }

    std::wstring DriveLetter::ToPath() const {
        if (!IsValid()) return L"";
        return std::wstring(1, m_letter) + L":\\";
    }

}
