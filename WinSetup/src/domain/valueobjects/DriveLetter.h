// src/domain/valueobjects/DriveLetter.h

#pragma once

#include <string>
#include <optional>

namespace winsetup::domain {

    class DriveLetter {
    public:
        DriveLetter() = default;

        explicit DriveLetter(wchar_t letter)
            : m_letter(Normalize(letter))
        {
        }

        explicit DriveLetter(const std::wstring& str) {
            if (!str.empty()) {
                m_letter = Normalize(str[0]);
            }
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_letter.has_value() &&
                m_letter.value() >= L'A' &&
                m_letter.value() <= L'Z';
        }

        [[nodiscard]] wchar_t GetLetter() const {
            if (!IsValid()) {
                throw std::logic_error("Invalid drive letter");
            }
            return m_letter.value();
        }

        [[nodiscard]] std::wstring ToString() const {
            if (!IsValid()) {
                return L"";
            }
            return std::wstring(1, m_letter.value());
        }

        [[nodiscard]] std::wstring ToPath() const {
            if (!IsValid()) {
                return L"";
            }
            return std::wstring(1, m_letter.value()) + L":\\";
        }

        bool operator==(const DriveLetter& other) const noexcept {
            return m_letter == other.m_letter;
        }

        bool operator!=(const DriveLetter& other) const noexcept {
            return !(*this == other);
        }

    private:
        static std::optional<wchar_t> Normalize(wchar_t letter) {
            if (letter >= L'a' && letter <= L'z') {
                return static_cast<wchar_t>(letter - L'a' + L'A');
            }
            if (letter >= L'A' && letter <= L'Z') {
                return letter;
            }
            return std::nullopt;
        }

        std::optional<wchar_t> m_letter;
    };

}
