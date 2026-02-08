// src/domain/valueobjects/DriveLetter.h

#pragma once

#include <string>
#include <stdexcept>
#include <cctype>

namespace winsetup::domain {

    class DriveLetter {
    public:
        DriveLetter() = default;

        explicit DriveLetter(wchar_t letter) {
            if (!IsValidLetter(letter)) {
                throw std::logic_error("Invalid drive letter");
            }
            m_letter = ToUpper(letter);
        }

        explicit DriveLetter(const std::wstring& str) {
            if (str.empty()) {
                throw std::logic_error("Empty drive letter string");
            }

            wchar_t letter = str[0];
            if (!IsValidLetter(letter)) {
                throw std::logic_error("Invalid drive letter");
            }
            m_letter = ToUpper(letter);
        }

        [[nodiscard]] wchar_t Get() const noexcept { return m_letter; }
        [[nodiscard]] std::wstring ToString() const {
            if (m_letter == L'\0') {
                return L"";
            }
            return std::wstring(1, m_letter) + L":";
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_letter != L'\0';
        }

        [[nodiscard]] static bool IsValidLetter(wchar_t letter) noexcept {
            wchar_t upper = ToUpper(letter);
            return upper >= L'A' && upper <= L'Z';
        }

        bool operator==(const DriveLetter& other) const noexcept {
            return m_letter == other.m_letter;
        }

        bool operator!=(const DriveLetter& other) const noexcept {
            return m_letter != other.m_letter;
        }

        bool operator<(const DriveLetter& other) const noexcept {
            return m_letter < other.m_letter;
        }

    private:
        [[nodiscard]] static wchar_t ToUpper(wchar_t c) noexcept {
            if (c >= L'a' && c <= L'z') {
                return c - (L'a' - L'A');
            }
            return c;
        }

        wchar_t m_letter = L'\0';
    };

}
