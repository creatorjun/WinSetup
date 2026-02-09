// src/domain/valueobjects/DriveLetter.h
#pragma once

#include <string>

namespace winsetup::domain {

    class DriveLetter {
    public:
        DriveLetter() = default;
        explicit DriveLetter(wchar_t letter) : m_letter(letter) {}

        [[nodiscard]] wchar_t GetLetter() const noexcept { return m_letter; }

        [[nodiscard]] std::wstring ToString() const {
            if (!IsValid()) return L"";
            return std::wstring(1, m_letter) + L":";
        }

        [[nodiscard]] std::wstring ToPath() const {
            if (!IsValid()) return L"";
            return std::wstring(1, m_letter) + L":\\";
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return (m_letter >= L'A' && m_letter <= L'Z') ||
                (m_letter >= L'a' && m_letter <= L'z');
        }

        [[nodiscard]] bool operator==(const DriveLetter& other) const noexcept {
            return m_letter == other.m_letter;
        }

        [[nodiscard]] bool operator!=(const DriveLetter& other) const noexcept {
            return m_letter != other.m_letter;
        }

        [[nodiscard]] bool operator<(const DriveLetter& other) const noexcept {
            return m_letter < other.m_letter;
        }

    private:
        wchar_t m_letter = L'\0';
    };

}
