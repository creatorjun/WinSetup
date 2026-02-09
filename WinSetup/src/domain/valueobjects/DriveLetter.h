// src/domain/valueobjects/DriveLetter.h
#pragma once

#include <string>
#include <optional>
#include "../primitives/Expected.h"

namespace winsetup::domain {

    class DriveLetter {
    public:
        DriveLetter() = default;

        [[nodiscard]] static Expected<DriveLetter> Create(wchar_t letter);

        [[nodiscard]] wchar_t GetLetter() const noexcept { return m_letter; }
        [[nodiscard]] std::wstring ToString() const;
        [[nodiscard]] std::wstring ToPath() const;

        [[nodiscard]] bool IsValid() const noexcept { return m_letter >= L'A' && m_letter <= L'Z'; }

        [[nodiscard]] bool operator==(const DriveLetter& other) const noexcept { return m_letter == other.m_letter; }
        [[nodiscard]] bool operator!=(const DriveLetter& other) const noexcept { return m_letter != other.m_letter; }
        [[nodiscard]] bool operator<(const DriveLetter& other) const noexcept { return m_letter < other.m_letter; }

    private:
        explicit DriveLetter(wchar_t letter) : m_letter(letter) {}
        wchar_t m_letter = L'\0';
    };

}
