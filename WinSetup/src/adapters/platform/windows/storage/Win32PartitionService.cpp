#include "Win32PartitionService.h"
#include <winioctl.h>
#include <ntdddisk.h>

#ifndef PARTITION_BASIC_DATA_GUID
static const GUID PARTITION_BASIC_DATA_GUID =
{ 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };
#endif

namespace winsetup::adapters {

    Win32PartitionService::Win32PartitionService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    )
        : textEncoder_(std::move(textEncoder)) {
    }

    domain::Expected<domain::PartitionInfo>
        Win32PartitionService::CreatePartition(
            domain::PhysicalDiskId diskId,
            const abstractions::PartitionLayout& layout
        ) noexcept {
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);
        if (handleResult.HasError()) [[unlikely]] {
            return domain::Expected<domain::PartitionInfo>::Failure(
                std::move(handleResult).GetError()
            );
        }

        HANDLE diskHandle = handleResult.Value();

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
            CloseHandle(diskHandle);
            return domain::Expected<domain::PartitionInfo>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
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
        }
        else {
            newPartition.Mbr.PartitionType = PARTITION_IFS;
            newPartition.Mbr.BootIndicator = layout.isBootable;
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

        return domain::Expected<domain::PartitionInfo>::Success(std::move(partInfo));
    }

    domain::Result<> Win32PartitionService::DeletePartition(
        domain::PartitionId partitionId
    ) noexcept {
        domain::PhysicalDiskId diskId{ partitionId.diskIndex };
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);

        if (handleResult.HasError()) [[unlikely]] {
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

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
                break;
            }
        }

        if (!found) [[unlikely]] {
            CloseHandle(diskHandle);
            return domain::Result<>::Failure(
                domain::Error("Partition not found")
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
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DRIVE_LAYOUT_EX")
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
        auto pathResult = GetPartitionPath(partitionId);
        if (pathResult.HasError()) [[unlikely]] {
            return domain::Result<>::Failure(std::move(pathResult).GetError());
        }

        std::wstring partitionPath = std::move(pathResult).Value();
        const wchar_t* fsName = GetFileSystemName(fileSystem);

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
            return domain::Result<>::Failure(
                CreateErrorFromLastError("CreateFileW")
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
            CloseHandle(volumeHandle);
            return domain::Result<>::Failure(
                CreateErrorFromLastError("FSCTL_LOCK_VOLUME")
            );
        }

        CloseHandle(volumeHandle);

        return domain::Result<>::Success();
    }

    domain::Result<> Win32PartitionService::SetPartitionActive(
        domain::PartitionId partitionId
    ) noexcept {
        domain::PhysicalDiskId diskId{ partitionId.diskIndex };
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);

        if (handleResult.HasError()) [[unlikely]] {
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

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
            CloseHandle(diskHandle);
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
            );
        }

        for (DWORD i = 0; i < layout.PartitionCount; ++i) {
            if (layout.PartitionEntry[i].PartitionNumber == partitionId.partitionNumber) {
                if (layout.PartitionStyle == PARTITION_STYLE_MBR) {
                    layout.PartitionEntry[i].Mbr.BootIndicator = TRUE;
                }
                layout.PartitionEntry[i].RewritePartition = TRUE;
            }
            else {
                if (layout.PartitionStyle == PARTITION_STYLE_MBR) {
                    layout.PartitionEntry[i].Mbr.BootIndicator = FALSE;
                }
            }
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
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DRIVE_LAYOUT_EX")
            );
        }

        return domain::Result<>::Success();
    }

    domain::Expected<uint64_t> Win32PartitionService::GetMaxPartitionSize(
        domain::PhysicalDiskId diskId
    ) const noexcept {
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ);
        if (handleResult.HasError()) [[unlikely]] {
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
            CloseHandle(diskHandle);
            return domain::Expected<uint64_t>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_LENGTH_INFO")
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

        uint64_t totalSize = static_cast<uint64_t>(lengthInfo.Length.QuadPart);
        uint64_t usedSize = 0;

        if (success) [[likely]] {
            for (DWORD i = 0; i < layout.PartitionCount; ++i) {
                usedSize += layout.PartitionEntry[i].PartitionLength.QuadPart;
            }
        }

        uint64_t maxSize = totalSize > usedSize ? totalSize - usedSize : 0;
        return domain::Expected<uint64_t>::Success(maxSize);
    }

    domain::Expected<HANDLE> Win32PartitionService::OpenDiskHandle(
        domain::PhysicalDiskId diskId,
        DWORD accessFlags
    ) const noexcept {
        std::wstring diskPath = diskId.ToString();

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
            return domain::Expected<HANDLE>::Failure(
                CreateErrorFromLastError("CreateFileW")
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
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    ) {
        return std::make_unique<Win32PartitionService>(std::move(textEncoder));
    }

}
