#pragma once

#include <memory>
#include "../../../../abstractions/storage/IDiskService.h"
#include "../../../../abstractions/platform/ITextEncoder.h"
#include "../../../../abstractions/logging/ILogger.h"
#include <Windows.h>

namespace winsetup::adapters {

    class Win32DiskService : public abstractions::IDiskService {
    public:
        explicit Win32DiskService(
            std::shared_ptr<abstractions::ITextEncoder> textEncoder,
            std::shared_ptr<abstractions::ILogger> logger
        );
        ~Win32DiskService() override = default;

        Win32DiskService(const Win32DiskService&) = delete;
        Win32DiskService& operator=(const Win32DiskService&) = delete;
        Win32DiskService(Win32DiskService&&) noexcept = default;
        Win32DiskService& operator=(Win32DiskService&&) noexcept = default;

        [[nodiscard]] domain::Expected<std::vector<domain::DiskInfo>>
            EnumeratePhysicalDisks() const noexcept override;

        [[nodiscard]] domain::Expected<domain::DiskInfo>
            GetDiskInfo(domain::PhysicalDiskId diskId) const noexcept override;

        [[nodiscard]] domain::Expected<std::vector<domain::PartitionInfo>>
            GetPartitions(domain::PhysicalDiskId diskId) const noexcept override;

        [[nodiscard]] domain::Result<>
            CleanDisk(domain::PhysicalDiskId diskId) noexcept override;

        [[nodiscard]] domain::Result<>
            InitializeDisk(
                domain::PhysicalDiskId diskId,
                domain::PartitionStyle style
            ) noexcept override;

        [[nodiscard]] domain::Result<>
            SetDiskOnline(domain::PhysicalDiskId diskId) noexcept override;

        [[nodiscard]] domain::Result<>
            SetDiskOffline(domain::PhysicalDiskId diskId) noexcept override;

        [[nodiscard]] domain::Expected<bool>
            IsDiskOnline(domain::PhysicalDiskId diskId) const noexcept override;

    private:
        [[nodiscard]] domain::Expected<HANDLE>
            OpenDiskHandle(domain::PhysicalDiskId diskId, DWORD accessFlags) const noexcept;

        [[nodiscard]] domain::Expected<domain::BusType>
            QueryBusType(HANDLE diskHandle) const noexcept;

        [[nodiscard]] domain::Expected<uint64_t>
            QueryDiskSize(HANDLE diskHandle) const noexcept;

        [[nodiscard]] domain::Expected<std::wstring>
            QueryDiskModel(HANDLE diskHandle) const noexcept;

        [[nodiscard]] domain::Expected<domain::PartitionStyle>
            QueryPartitionStyle(HANDLE diskHandle) const noexcept;

        [[nodiscard]] domain::Error
            CreateErrorFromLastError(std::string_view operation) const noexcept;

        std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
        std::shared_ptr<abstractions::ILogger> logger_;
    };

    std::unique_ptr<abstractions::IDiskService> CreateDiskService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    );

}
