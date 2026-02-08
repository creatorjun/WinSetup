// src/domain/valueobjects/DiskSize.h

#pragma once

#include <cstdint>

namespace winsetup::domain {

    class DiskSize {
    public:
        static constexpr uint64_t BYTES_PER_KB = 1024ULL;
        static constexpr uint64_t BYTES_PER_MB = 1024ULL * 1024ULL;
        static constexpr uint64_t BYTES_PER_GB = 1024ULL * 1024ULL * 1024ULL;
        static constexpr uint64_t BYTES_PER_TB = 1024ULL * 1024ULL * 1024ULL * 1024ULL;

        constexpr DiskSize() noexcept : m_bytes(0) {}

        constexpr explicit DiskSize(uint64_t bytes) noexcept : m_bytes(bytes) {}

        [[nodiscard]] static constexpr DiskSize FromBytes(uint64_t bytes) noexcept {
            return DiskSize(bytes);
        }

        [[nodiscard]] static constexpr DiskSize FromKB(uint64_t kb) noexcept {
            return DiskSize(kb * BYTES_PER_KB);
        }

        [[nodiscard]] static constexpr DiskSize FromMB(uint64_t mb) noexcept {
            return DiskSize(mb * BYTES_PER_MB);
        }

        [[nodiscard]] static constexpr DiskSize FromGB(uint64_t gb) noexcept {
            return DiskSize(gb * BYTES_PER_GB);
        }

        [[nodiscard]] static constexpr DiskSize FromTB(uint64_t tb) noexcept {
            return DiskSize(tb * BYTES_PER_TB);
        }

        [[nodiscard]] constexpr uint64_t ToBytes() const noexcept {
            return m_bytes;
        }

        [[nodiscard]] constexpr double ToKB() const noexcept {
            return static_cast<double>(m_bytes) / BYTES_PER_KB;
        }

        [[nodiscard]] constexpr double ToMB() const noexcept {
            return static_cast<double>(m_bytes) / BYTES_PER_MB;
        }

        [[nodiscard]] constexpr double ToGB() const noexcept {
            return static_cast<double>(m_bytes) / BYTES_PER_GB;
        }

        [[nodiscard]] constexpr double ToTB() const noexcept {
            return static_cast<double>(m_bytes) / BYTES_PER_TB;
        }

        constexpr bool operator==(const DiskSize& other) const noexcept {
            return m_bytes == other.m_bytes;
        }

        constexpr bool operator!=(const DiskSize& other) const noexcept {
            return m_bytes != other.m_bytes;
        }

        constexpr bool operator<(const DiskSize& other) const noexcept {
            return m_bytes < other.m_bytes;
        }

        constexpr bool operator<=(const DiskSize& other) const noexcept {
            return m_bytes <= other.m_bytes;
        }

        constexpr bool operator>(const DiskSize& other) const noexcept {
            return m_bytes > other.m_bytes;
        }

        constexpr bool operator>=(const DiskSize& other) const noexcept {
            return m_bytes >= other.m_bytes;
        }

        constexpr DiskSize operator+(const DiskSize& other) const noexcept {
            return DiskSize(m_bytes + other.m_bytes);
        }

        constexpr DiskSize operator-(const DiskSize& other) const noexcept {
            return DiskSize(m_bytes > other.m_bytes ? m_bytes - other.m_bytes : 0);
        }

    private:
        uint64_t m_bytes;
    };

}
