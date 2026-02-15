// src/adapters/platform/win32/storage/Win32DiskService.h
#pragma once

#include <abstractions/services/storage/IDiskService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <adapters/platform/win32/core/UniqueHandle.h>
#include <Windows.h>
#include <memory>
#include <vector>

namespace winsetup::adapters::platform {

    class Win32DiskService : public abstractions::IDiskService {
    public:
        explicit Win32DiskService(std::shared_ptr<abstractions::ILogger> logger);
        ~Win32DiskService() override = default;

        [[nodiscard]] domain::Expected<std::vector<domain::DiskInfo>>
            EnumerateDisks() override;

        [[nodiscard]] domain::Expected<domain::DiskInfo>
            GetDiskInfo(uint32_t diskIndex) override;

        [[nodiscard]] domain::Expected<void>
            CleanDisk(uint32_t diskIndex) override;

        [[nodiscard]] domain::Expected<void>
            CreatePartitionLayout(
                uint32_t diskIndex,
                const abstractions::PartitionLayout& layout
            ) override;

        [[nodiscard]] domain::Expected<void>
            FormatPartition(
                uint32_t diskIndex,
                uint32_t partitionIndex,
                domain::FileSystemType fileSystem,
                bool quickFormat = true
            ) override;

        [[nodiscard]] domain::Expected<abstractions::PartitionLayout>
            GetCurrentLayout(uint32_t diskIndex) override;

        [[nodiscard]] domain::Expected<void>
            RestoreLayout(
                uint32_t diskIndex,
                const abstractions::PartitionLayout& layout
            ) override;

    private:
        [[nodiscard]] adapters::platform::UniqueHandle OpenDiskHandle(uint32_t diskIndex);

        [[nodiscard]] domain::Expected<DISK_GEOMETRY_EX> GetDiskGeometry(HANDLE hDisk);

        [[nodiscard]] domain::Expected<std::wstring> GetPartitionVolumePath(
            uint32_t diskIndex,
            uint32_t partitionIndex
        );

        std::shared_ptr<abstractions::ILogger> mLogger;
    };

}
