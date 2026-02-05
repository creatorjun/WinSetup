#include "Win32PartitionService.h"
#include <winioctl.h>
#include <ntdddisk.h>

#ifndef PARTITION_BASIC_DATA_GUID
static const GUID PARTITION_BASIC_DATA_GUID =
{ 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };
#endif

namespace winsetup::adapters {

    Win32PartitionService::Win32PartitionService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : textEncoder_(std::move(textEncoder))
        , logger_(std::move(logger)) {
        if (logger_) {
            logger_->Log(abstractions::LogLevel::Info, L"Win32PartitionService initialized");
        }
    }

    domain::Expected<domain::PartitionInfo>
        Win32PartitionService::CreatePartition(
            domain::PhysicalDiskId diskId,
            const abstractions::PartitionLayout& layout
        ) noexcept {
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Creating partition on disk " + std::to_wstring(diskId.index) +
                L" with size " + std::to_wstring(layout.sizeInBytes / (1024 * 1024)) + L" MB"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);
        if (handleResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk handle for disk " + std::to_wstring(diskId.index)
                );
            }
            return domain::Expected<domain::PartitionInfo>::Failure(
                std::move(handleResult).GetError()
            );
        }

        HANDLE diskHandle = handleResult.Value();

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Retrieving current partition layout for disk " + std::to_wstring(diskId.index)
            );
        }

        DRIVE_LAYOUT_INFORMATION_EX currentLayout{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr,
            0,
            &currentLayout,
            sizeof(currentLayout),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed with error " + std::to_wstring(errorCode)
                );
            }
            CloseHandle(diskHandle);
            return domain::Expected<domain::PartitionInfo>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Current partition count: " + std::to_wstring(currentLayout.PartitionCount) +
                L", Style: " + (currentLayout.PartitionStyle == PARTITION_STYLE_GPT ? L"GPT" : L"MBR")
            );
        }

        DRIVE_LAYOUT_INFORMATION_EX newLayout = currentLayout;
        newLayout.PartitionCount++;

        auto& newPartition = newLayout.PartitionEntry[newLayout.PartitionCount - 1];
        newPartition.PartitionStyle = static_cast<PARTITION_STYLE>(currentLayout.PartitionStyle);
        newPartition.StartingOffset.QuadPart = 1024 * 1024;
        newPartition.PartitionLength.QuadPart = layout.sizeInBytes;
        newPartition.PartitionNumber = newLayout.PartitionCount;
        newPartition.RewritePartition = TRUE;

        if (currentLayout.PartitionStyle == PARTITION_STYLE_GPT) {
            CoCreateGuid(&newPartition.Gpt.PartitionId);
            newPartition.Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
            wcscpy_s(newPartition.Gpt.Name, layout.label.c_str());
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"Creating GPT partition with label: " + layout.label
                );
            }
        }
        else {
            newPartition.Mbr.PartitionType = PARTITION_IFS;
            newPartition.Mbr.BootIndicator = layout.isBootable;
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"Creating MBR partition, bootable: " + std::wstring(layout.isBootable ? L"Yes" : L"No")
                );
            }
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Applying new partition layout to disk " + std::to_wstring(diskId.index)
            );
        }

        success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            &newLayout,
            sizeof(newLayout),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX failed with error " + std::to_wstring(errorCode)
                );
            }
            return domain::Expected<domain::PartitionInfo>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DRIVE_LAYOUT_EX")
            );
        }

        domain::PartitionId partId{ diskId.index, newLayout.PartitionCount };
        domain::PartitionInfo partInfo(
            partId,
            newPartition.StartingOffset.QuadPart,
            newPartition.PartitionLength.QuadPart,
            domain::PartitionType::Basic
        );

        partInfo.SetBootable(layout.isBootable);

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Partition created successfully: Disk " + std::to_wstring(diskId.index) +
                L", Partition " + std::to_wstring(newLayout.PartitionCount)
            );
        }

        return domain::Expected<domain::PartitionInfo>::Success(std::move(partInfo));
    }

    domain::Result<> Win32PartitionService::DeletePartition(
        domain::PartitionId partitionId
    ) noexcept {
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Deleting partition: Disk " + std::to_wstring(partitionId.diskIndex) +
                L", Partition " + std::to_wstring(partitionId.partitionNumber)
            );
        }

        domain::PhysicalDiskId diskId{ partitionId.diskIndex };
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);

        if (handleResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk handle for partition deletion"
                );
            }
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Retrieving partition layout for deletion"
            );
        }

        DRIVE_LAYOUT_INFORMATION_EX layout{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr,
            0,
            &layout,
            sizeof(layout),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed with error " + std::to_wstring(errorCode)
                );
            }
            CloseHandle(diskHandle);
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
            );
        }

        bool found = false;
        for (DWORD i = 0; i < layout.PartitionCount; ++i) {
            if (layout.PartitionEntry[i].PartitionNumber == partitionId.partitionNumber) {
                layout.PartitionEntry[i].RewritePartition = TRUE;
                layout.PartitionEntry[i].PartitionLength.QuadPart = 0;
                found = true;
                if (logger_) {
                    logger_->Log(
                        abstractions::LogLevel::Debug,
                        L"Target partition found at index " + std::to_wstring(i)
                    );
                }
                break;
            }
        }

        if (!found) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Warning,
                    L"Partition " + std::to_wstring(partitionId.partitionNumber) + L" not found on disk"
                );
            }
            CloseHandle(diskHandle);
            return domain::Result<>::Failure(
                domain::Error("Partition not found")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Applying partition deletion to disk"
            );
        }

        success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            &layout,
            sizeof(layout),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to apply partition deletion, error " + std::to_wstring(errorCode)
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DRIVE_LAYOUT_EX")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Partition deleted successfully"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32PartitionService::FormatPartition(
        domain::PartitionId partitionId,
        domain::FileSystemType fileSystem,
        const std::wstring& label,
        bool quickFormat
    ) noexcept {
        const wchar_t* fsName = GetFileSystemName(fileSystem);
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Formatting partition " + std::to_wstring(partitionId.partitionNumber) +
                L" on disk " + std::to_wstring(partitionId.diskIndex) +
                L" as " + std::wstring(fsName) +
                L", label: " + label +
                L", quick: " + std::wstring(quickFormat ? L"Yes" : L"No")
            );
        }

        auto pathResult = GetPartitionPath(partitionId);
        if (pathResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to get partition path"
                );
            }
            return domain::Result<>::Failure(std::move(pathResult).GetError());
        }

        std::wstring partitionPath = std::move(pathResult).Value();
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Partition path: " + partitionPath
            );
        }

        HANDLE volumeHandle = CreateFileW(
            partitionPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (volumeHandle == INVALID_HANDLE_VALUE) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open volume handle, error " + std::to_wstring(errorCode)
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("CreateFileW")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Locking volume for format operation"
            );
        }

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            volumeHandle,
            FSCTL_LOCK_VOLUME,
            nullptr,
            0,
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"FSCTL_LOCK_VOLUME failed with error " + std::to_wstring(errorCode)
                );
            }
            CloseHandle(volumeHandle);
            return domain::Result<>::Failure(
                CreateErrorFromLastError("FSCTL_LOCK_VOLUME")
            );
        }

        CloseHandle(volumeHandle);

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Volume locked successfully, format operation ready"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32PartitionService::SetPartitionActive(
        domain::PartitionId partitionId
    ) noexcept {
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Setting partition " + std::to_wstring(partitionId.partitionNumber) +
                L" on disk " + std::to_wstring(partitionId.diskIndex) + L" as active"
            );
        }

        domain::PhysicalDiskId diskId{ partitionId.diskIndex };
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);

        if (handleResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk handle for setting active partition"
                );
            }
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Retrieving current partition layout"
            );
        }

        DRIVE_LAYOUT_INFORMATION_EX layout{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr,
            0,
            &layout,
            sizeof(layout),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed with error " + std::to_wstring(errorCode)
                );
            }
            CloseHandle(diskHandle);
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
            );
        }

        if (layout.PartitionStyle == PARTITION_STYLE_GPT) {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Warning,
                    L"Cannot set active partition on GPT disk (boot flag not applicable)"
                );
            }
        }

        for (DWORD i = 0; i < layout.PartitionCount; ++i) {
            if (layout.PartitionEntry[i].PartitionNumber == partitionId.partitionNumber) {
                if (layout.PartitionStyle == PARTITION_STYLE_MBR) {
                    layout.PartitionEntry[i].Mbr.BootIndicator = TRUE;
                    if (logger_) {
                        logger_->Log(
                            abstractions::LogLevel::Debug,
                            L"Setting boot indicator for partition " + std::to_wstring(i)
                        );
                    }
                }
                layout.PartitionEntry[i].RewritePartition = TRUE;
            }
            else {
                if (layout.PartitionStyle == PARTITION_STYLE_MBR) {
                    layout.PartitionEntry[i].Mbr.BootIndicator = FALSE;
                }
            }
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Applying active partition changes"
            );
        }

        success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            &layout,
            sizeof(layout),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to set active partition, error " + std::to_wstring(errorCode)
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DRIVE_LAYOUT_EX")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Partition set as active successfully"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Expected<uint64_t> Win32PartitionService::GetMaxPartitionSize(
        domain::PhysicalDiskId diskId
    ) const noexcept {
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Calculating maximum partition size for disk " + std::to_wstring(diskId.index)
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ);
        if (handleResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk handle for size calculation"
                );
            }
            return domain::Expected<uint64_t>::Failure(
                std::move(handleResult).GetError()
            );
        }

        HANDLE diskHandle = handleResult.Value();

        GET_LENGTH_INFORMATION lengthInfo{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_LENGTH_INFO,
            nullptr,
            0,
            &lengthInfo,
            sizeof(lengthInfo),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"IOCTL_DISK_GET_LENGTH_INFO failed with error " + std::to_wstring(errorCode)
                );
            }
            CloseHandle(diskHandle);
            return domain::Expected<uint64_t>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_LENGTH_INFO")
            );
        }

        uint64_t totalSize = static_cast<uint64_t>(lengthInfo.Length.QuadPart);
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Total disk size: " + std::to_wstring(totalSize / (1024 * 1024 * 1024)) + L" GB"
            );
        }

        DRIVE_LAYOUT_INFORMATION_EX layout{};
        success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr,
            0,
            &layout,
            sizeof(layout),
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        uint64_t usedSize = 0;

        if (success) [[likely]] {
            for (DWORD i = 0; i < layout.PartitionCount; ++i) {
                usedSize += layout.PartitionEntry[i].PartitionLength.QuadPart;
            }
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"Used space: " + std::to_wstring(usedSize / (1024 * 1024 * 1024)) + L" GB"
                );
            }
        }

        uint64_t maxSize = totalSize > usedSize ? totalSize - usedSize : 0;
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Maximum available partition size: " + std::to_wstring(maxSize / (1024 * 1024 * 1024)) + L" GB"
            );
        }

        return domain::Expected<uint64_t>::Success(maxSize);
    }

    domain::Expected<HANDLE> Win32PartitionService::OpenDiskHandle(
        domain::PhysicalDiskId diskId,
        DWORD accessFlags
    ) const noexcept {
        std::wstring diskPath = diskId.ToString();

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Opening disk handle: " + diskPath
            );
        }

        HANDLE handle = CreateFileW(
            diskPath.c_str(),
            accessFlags,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (handle == INVALID_HANDLE_VALUE) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk handle: " + diskPath + L", error " + std::to_wstring(errorCode)
                );
            }
            return domain::Expected<HANDLE>::Failure(
                CreateErrorFromLastError("CreateFileW")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Disk handle opened successfully"
            );
        }

        return domain::Expected<HANDLE>::Success(handle);
    }

    domain::Expected<std::wstring> Win32PartitionService::GetPartitionPath(
        domain::PartitionId partitionId
    ) const noexcept {
        std::wstring path = L"\\\\.\\PhysicalDrive";
        path.append(std::to_wstring(partitionId.diskIndex));
        path.append(L"Partition");
        path.append(std::to_wstring(partitionId.partitionNumber));

        return domain::Expected<std::wstring>::Success(std::move(path));
    }

    const wchar_t* Win32PartitionService::GetFileSystemName(
        domain::FileSystemType fsType
    ) const noexcept {
        switch (fsType) {
        case domain::FileSystemType::NTFS:
            return L"NTFS";
        case domain::FileSystemType::FAT32:
            return L"FAT32";
        case domain::FileSystemType::exFAT:
            return L"exFAT";
        case domain::FileSystemType::ReFS:
            return L"ReFS";
        default:
            return L"NTFS";
        }
    }

    domain::Error Win32PartitionService::CreateErrorFromLastError(
        std::string_view operation
    ) const noexcept {
        const DWORD errorCode = ::GetLastError();

        std::string message = "Win32PartitionService: ";
        message.append(operation);
        message.append(" failed with error code: ");
        message.append(std::to_string(errorCode));

        return domain::Error(std::move(message), static_cast<int>(errorCode));
    }

    std::unique_ptr<abstractions::IPartitionService> CreatePartitionService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    ) {
        return std::make_unique<Win32PartitionService>(std::move(textEncoder), std::move(logger));
    }

}
