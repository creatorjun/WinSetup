// src/adapters/platform/win32/core/Win32TypeMapper.h
#pragma once

#include "../../../../domain/valueobjects/BusType.h"
#include "../../../../domain/valueobjects/DiskType.h"
#include "../../../../domain/valueobjects/FileSystemType.h"
#include "../../../../domain/valueobjects/PartitionType.h"
#include <Windows.h>
#include <winioctl.h>

namespace winsetup::adapters::platform {

    class Win32TypeMapper {
    public:
        [[nodiscard]] static domain::BusType MapBusType(STORAGE_BUS_TYPE winBusType) noexcept;

        [[nodiscard]] static domain::DiskType MapDiskType(
            MEDIA_TYPE mediaType,
            STORAGE_BUS_TYPE busType,
            bool hasSeekPenalty
        ) noexcept;

        [[nodiscard]] static domain::FileSystemType MapFileSystemType(
            const std::wstring& fileSystemName
        ) noexcept;

        [[nodiscard]] static domain::PartitionType MapPartitionType(
            BYTE partitionType,
            bool isGPT
        ) noexcept;

        [[nodiscard]] static STORAGE_BUS_TYPE MapToWin32BusType(
            domain::BusType busType
        ) noexcept;

        [[nodiscard]] static MEDIA_TYPE MapToWin32MediaType(
            domain::DiskType diskType
        ) noexcept;

    private:
        Win32TypeMapper() = delete;
    };

}
