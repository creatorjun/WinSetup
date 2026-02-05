#pragma once

#include <memory>
#include <Windows.h>
#include "../../../../abstractions/storage/IPartitionService.h"
#include "../../../../abstractions/platform/ITextEncoder.h"
#include "../../../../abstractions/logging/ILogger.h"
#include "../../../../domain/primitives/LogLevel.h" 

namespace winsetup::adapters {

    class Win32PartitionService : public abstractions::IPartitionService {
    public:
        explicit Win32PartitionService(
            std::shared_ptr<abstractions::ITextEncoder> textEncoder,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<domain::PartitionInfo>
            CreatePartition(
                domain::PhysicalDiskId diskId,
                const abstractions::PartitionLayout& layout
            ) noexcept override;

        [[nodiscard]] domain::Result<>
            DeletePartition(domain::PartitionId partitionId) noexcept override;

        [[nodiscard]] domain::Result<>
            FormatPartition(
                domain::PartitionId partitionId,
                domain::FileSystemType fileSystem,
                const std::wstring& label,
                bool quickFormat = true
            ) noexcept override;

        [[nodiscard]] domain::Result<>
            SetPartitionActive(domain::PartitionId partitionId) noexcept override;

        [[nodiscard]] domain::Expected<uint64_t>
            GetMaxPartitionSize(domain::PhysicalDiskId diskId) const noexcept override;

    private:
        [[nodiscard]] domain::Expected<HANDLE>
            OpenDiskHandle(
                domain::PhysicalDiskId diskId,
                DWORD accessFlags
            ) const noexcept;

        [[nodiscard]] domain::Expected<std::wstring>
            GetPartitionPath(domain::PartitionId partitionId) const noexcept;

        [[nodiscard]] const wchar_t*
            GetFileSystemName(domain::FileSystemType fsType) const noexcept;

        [[nodiscard]] domain::Error
            CreateErrorFromLastError(std::string_view operation) const noexcept;

        std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
        std::shared_ptr<abstractions::ILogger> logger_;
    };

    std::unique_ptr<abstractions::IPartitionService> CreatePartitionService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    );

}
