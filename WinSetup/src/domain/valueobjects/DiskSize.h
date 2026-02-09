// src/domain/valueobjects/DiskSize.h
#pragma once

#include <cstdint>
#include <string>

namespace winsetup::domain {

    class DiskSize {
    public:
        static constexpr uint64_t KB = 1024ULL;
        static constexpr uint64_t MB = KB * 1024ULL;
        static constexpr uint64_t GB = MB * 1024ULL;
        static constexpr uint64_t TB = GB * 1024ULL;

        constexpr DiskSize() noexcept : m_bytes(0) {}
        constexpr explicit DiskSize(uint64_t bytes) noexcept : m_bytes(bytes) {}

        [[nodiscard]] constexpr uint64_t ToBytes() const noexcept { return m_bytes; }
        [[nodiscard]] constexpr double ToKB() const noexcept { return static_cast<double>(m_bytes) / KB; }
        [[nodiscard]] constexpr double ToMB() const noexcept { return static_cast<double>(m_bytes) / MB; }
        [[nodiscard]] constexpr double ToGB() const noexcept { return static_cast<double>(m_bytes) / GB; }
        [[nodiscard]] constexpr double ToTB() const noexcept { return static_cast<double>(m_bytes) / TB; }

        [[nodiscard]] static constexpr DiskSize FromBytes(uint64_t bytes) noexcept { return DiskSize(bytes); }
        [[nodiscard]] static constexpr DiskSize FromKB(uint64_t kb) noexcept { return DiskSize(kb * KB); }
        [[nodiscard]] static constexpr DiskSize FromMB(uint64_t mb) noexcept { return DiskSize(mb * MB); }
        [[nodiscard]] static constexpr DiskSize FromGB(uint64_t gb) noexcept { return DiskSize(gb * GB); }
        [[nodiscard]] static constexpr DiskSize FromTB(uint64_t tb) noexcept { return DiskSize(tb * TB); }

        [[nodiscard]] std::wstring ToString() const;

        [[nodiscard]] constexpr bool operator==(const DiskSize& other) const noexcept { return m_bytes == other.m_bytes; }
        [[nodiscard]] constexpr bool operator!=(const DiskSize& other) const noexcept { return m_bytes != other.m_bytes; }
        [[nodiscard]] constexpr bool operator<(const DiskSize& other) const noexcept { return m_bytes < other.m_bytes; }
        [[nodiscard]] constexpr bool operator<=(const DiskSize& other) const noexcept { return m_bytes <= other.m_bytes; }
        [[nodiscard]] constexpr bool operator>(const DiskSize& other) const noexcept { return m_bytes > other.m_bytes; }
        [[nodiscard]] constexpr bool operator>=(const DiskSize& other) const noexcept { return m_bytes >= other.m_bytes; }

        [[nodiscard]] constexpr DiskSize operator+(const DiskSize& other) const noexcept { return DiskSize(m_bytes + other.m_bytes); }
        [[nodiscard]] constexpr DiskSize operator-(const DiskSize& other) const noexcept { return DiskSize(m_bytes - other.m_bytes); }
        [[nodiscard]] constexpr DiskSize operator*(uint64_t multiplier) const noexcept { return DiskSize(m_bytes * multiplier); }
        [[nodiscard]] constexpr DiskSize operator/(uint64_t divisor) const noexcept { return DiskSize(m_bytes / divisor); }

    private:
        uint64_t m_bytes;
    };

}
