// src/domain/entities/DiskInfo.h

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "VolumeInfo.h"
#include <domain/valueobjects/BusType.h>
#include <domain/valueobjects/DiskSize.h>

namespace winsetup::domain {

    class DiskInfo {
    public:
        DiskInfo() = default;

        DiskInfo(int index, const std::wstring& type, DiskSize size)
            : m_index(index)
            , m_typeString(type)
            , m_typeEnum(ParseDiskType(type))
            , m_size(size)
        {
        }

        [[nodiscard]] int GetIndex() const noexcept { return m_index; }
        [[nodiscard]] const std::wstring& GetType() const noexcept { return m_typeString; }
        [[nodiscard]] DiskTypeEnum GetTypeEnum() const noexcept { return m_typeEnum; }
        [[nodiscard]] int GetTypePriority() const noexcept {
            return static_cast<int>(m_typeEnum);
        }
        [[nodiscard]] DiskSize GetSize() const noexcept { return m_size; }
        [[nodiscard]] const std::vector<VolumeInfo>& GetVolumes() const noexcept {
            return m_volumes;
        }

        void SetIndex(int index) noexcept { m_index = index; }

        void SetType(const std::wstring& type) {
            m_typeString = type;
            m_typeEnum = ParseDiskType(type);
        }

        void SetSize(DiskSize size) noexcept { m_size = size; }

        void AddVolume(const VolumeInfo& volume) {
            m_volumes.push_back(volume);
        }

        void ClearVolumes() {
            m_volumes.clear();
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_index >= 0 && m_size.ToBytes() > 0;
        }

        [[nodiscard]] bool IsSSD() const noexcept {
            return m_typeEnum == DiskTypeEnum::NVME ||
                m_typeEnum == DiskTypeEnum::SSD;
        }

        [[nodiscard]] bool IsHDD() const noexcept {
            return m_typeEnum == DiskTypeEnum::HDD;
        }

        [[nodiscard]] bool HasEnoughSpace(DiskSize required) const noexcept {
            return m_size >= required;
        }

        [[nodiscard]] size_t GetVolumeCount() const noexcept {
            return m_volumes.size();
        }

        [[nodiscard]] const VolumeInfo* FindVolumeByLetter(const std::wstring& letter) const {
            auto it = std::find_if(m_volumes.begin(), m_volumes.end(),
                [&letter](const VolumeInfo& vol) {
                    return vol.GetLetter() == letter;
                });

            return it != m_volumes.end() ? &(*it) : nullptr;
        }

        [[nodiscard]] double GetSizeGB() const noexcept {
            return m_size.ToGB();
        }

        static constexpr int INVALID_DISK_NUMBER = -1;

    private:
        int m_index = INVALID_DISK_NUMBER;
        std::wstring m_typeString;
        DiskTypeEnum m_typeEnum = DiskTypeEnum::UNKNOWN;
        DiskSize m_size;
        std::vector<VolumeInfo> m_volumes;
    };

}
