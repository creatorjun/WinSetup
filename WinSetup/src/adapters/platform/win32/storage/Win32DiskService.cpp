// src/adapters/platform/win32/storage/Win32DiskService.cpp
#include "Win32DiskService.h"
#include "../core/Win32HandleFactory.h"
#include "../core/Win32ErrorHandler.h"
#include "../core/Win32StringHelper.h"
#include <Windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <shellapi.h>
#include <setupapi.h>
#include <devguid.h>
#include <vector>
#include <algorithm>
#include <sstream>

#pragma comment(lib, "setupapi.lib")

namespace winsetup::adapters::platform {

    namespace {
        constexpr GUID PARTITION_SYSTEM_GUID =
        { 0xC12A7328, 0xF81F, 0x11D2, { 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B } };

        constexpr GUID PARTITION_MSFT_RESERVED_GUID =
        { 0xE3C9E316, 0x0B5C, 0x4DB8, { 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE } };

        constexpr GUID PARTITION_BASIC_DATA_GUID =
        { 0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } };

        constexpr DWORD FSCTL_FORMAT_VOLUME = 0x000902EC;

        std::wstring FormatDiskPath(uint32_t diskIndex) {
            return L"\\\\.\\PhysicalDrive" + std::to_wstring(diskIndex);
        }

        std::wstring FormatMessage(const std::wstring& format, uint32_t value) {
            std::wstringstream ss;
            size_t pos = format.find(L"{}");
            if (pos != std::wstring::npos)
                ss << format.substr(0, pos) << value << format.substr(pos + 2);
            else
                ss << format;
            return ss.str();
        }

        std::wstring FormatMessage(const std::wstring& format, size_t value1, uint32_t value2) {
            std::wstringstream ss;
            std::wstring temp = format;

            size_t pos = temp.find(L"{}");
            if (pos != std::wstring::npos) {
                ss << temp.substr(0, pos) << value1;
                temp = temp.substr(pos + 2);
            }

            pos = temp.find(L"{}");
            if (pos != std::wstring::npos)
                ss << temp.substr(0, pos) << value2 << temp.substr(pos + 2);
            else
                ss << temp;

            return ss.str();
        }

        std::wstring ConcatWithCount(const std::wstring& prefix, size_t count, const std::wstring& suffix) {
            return prefix + std::to_wstring(count) + suffix;
        }

        bool AreGuidsEqual(const GUID& g1, const GUID& g2) {
            return memcmp(&g1, &g2, sizeof(GUID)) == 0;
        }

        domain::BusType MapBusType(STORAGE_BUS_TYPE windowsBusType) {
            switch (windowsBusType) {
            case BusTypeSata: return domain::BusType::SATA;
            case BusTypeNvme: return domain::BusType::NVME;
            case BusTypeUsb:  return domain::BusType::USB;
            case BusTypeScsi: return domain::BusType::SCSI;
            default:          return domain::BusType::Unknown;
            }
        }

        std::vector<uint32_t> EnumerateDiskIndicesViaSetupAPI() {
            std::vector<uint32_t> indices;

            HDEVINFO hDevInfo = SetupDiGetClassDevsW(
                &GUID_DEVINTERFACE_DISK,
                nullptr, nullptr,
                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
            );

            if (hDevInfo == INVALID_HANDLE_VALUE)
                return indices;

            SP_DEVICE_INTERFACE_DATA ifData{};
            ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            for (DWORD j = 0; SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &GUID_DEVINTERFACE_DISK, j, &ifData); ++j) {
                DWORD requiredSize = 0;
                SetupDiGetDeviceInterfaceDetailW(hDevInfo, &ifData, nullptr, 0, &requiredSize, nullptr);

                std::vector<BYTE> detailBuffer(requiredSize);
                auto detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuffer.data());
                detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

                if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo, &ifData, detail, requiredSize, nullptr, nullptr))
                    continue;

                HANDLE hDisk = CreateFileW(
                    detail->DevicePath,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, 0, nullptr
                );

                if (hDisk == INVALID_HANDLE_VALUE)
                    continue;

                STORAGE_DEVICE_NUMBER sdn{};
                DWORD bytesReturned = 0;
                if (DeviceIoControl(hDisk, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                    nullptr, 0, &sdn, sizeof(sdn), &bytesReturned, nullptr))
                {
                    if (sdn.DeviceType == FILE_DEVICE_DISK)
                        indices.push_back(sdn.DeviceNumber);
                }

                CloseHandle(hDisk);
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);

            std::sort(indices.begin(), indices.end());
            indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

            return indices;
        }
    }

    Win32DiskService::Win32DiskService(std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger))
    {
        if (mLogger)
            mLogger->Info(L"Win32DiskService initialized");
    }

    adapters::platform::UniqueHandle Win32DiskService::OpenDiskHandle(uint32_t diskIndex) {
        std::wstring path = FormatDiskPath(diskIndex);

        HANDLE hDisk = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (hDisk == INVALID_HANDLE_VALUE)
            return adapters::platform::UniqueHandle();

        return Win32HandleFactory::MakeHandle(hDisk);
    }

    domain::Expected<std::vector<domain::DiskInfo>> Win32DiskService::EnumerateDisks() {
        if (mLogger)
            mLogger->Debug(L"Enumerating disks...");

        std::vector<domain::DiskInfo> disks;

        const auto indices = EnumerateDiskIndicesViaSetupAPI();

        for (uint32_t index : indices) {
            auto diskInfoResult = GetDiskInfo(index);
            if (diskInfoResult.HasValue())
                disks.push_back(std::move(diskInfoResult.Value()));
        }

        if (mLogger)
            mLogger->Info(ConcatWithCount(L"Found ", disks.size(), L" disks"));

        return disks;
    }

    domain::Expected<domain::DiskInfo> Win32DiskService::GetDiskInfo(uint32_t diskIndex) {
        auto handle = OpenDiskHandle(diskIndex);
        if (!handle) {
            return domain::Error{
                FormatMessage(L"Failed to open disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        HANDLE hDisk = Win32HandleFactory::ToWin32Handle(handle);

        DISK_GEOMETRY_EX geometry{};
        DWORD bytesReturned = 0;

        BOOL result = DeviceIoControl(
            hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            nullptr, 0,
            &geometry, sizeof(geometry),
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                FormatMessage(L"IOCTL_DISK_GET_DRIVE_GEOMETRY_EX failed for disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        STORAGE_DEVICE_NUMBER deviceNumber{};
        result = DeviceIoControl(
            hDisk, IOCTL_STORAGE_GET_DEVICE_NUMBER,
            nullptr, 0,
            &deviceNumber, sizeof(deviceNumber),
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                FormatMessage(L"IOCTL_STORAGE_GET_DEVICE_NUMBER failed for disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        alignas(8) BYTE buffer[4096]{};
        result = DeviceIoControl(
            hDisk, IOCTL_STORAGE_QUERY_PROPERTY,
            &query, sizeof(query),
            buffer, sizeof(buffer),
            &bytesReturned, nullptr
        );

        domain::DiskType diskType = domain::DiskType::HDD;
        domain::BusType  busType = domain::BusType::Unknown;

        if (result) {
            auto descriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
            busType = MapBusType(static_cast<STORAGE_BUS_TYPE>(descriptor->BusType));

            DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenalty{};
            STORAGE_PROPERTY_QUERY seekQuery{};
            seekQuery.PropertyId = StorageDeviceSeekPenaltyProperty;
            seekQuery.QueryType = PropertyStandardQuery;

            result = DeviceIoControl(
                hDisk, IOCTL_STORAGE_QUERY_PROPERTY,
                &seekQuery, sizeof(seekQuery),
                &seekPenalty, sizeof(seekPenalty),
                &bytesReturned, nullptr
            );

            if (result && !seekPenalty.IncursSeekPenalty)
                diskType = domain::DiskType::SSD;
        }

        domain::DiskInfo diskInfo;
        diskInfo.SetIndex(diskIndex);
        diskInfo.SetSize(domain::DiskSize::FromBytes(geometry.DiskSize.QuadPart));
        diskInfo.SetDiskType(diskType);
        diskInfo.SetBusType(busType);

        auto layoutResult = GetCurrentLayout(diskIndex);
        if (layoutResult.HasValue()) {
            for (const auto& partition : layoutResult.Value().partitions)
                diskInfo.AddPartition(partition);
        }

        return diskInfo;
    }

    domain::Expected<void> Win32DiskService::CleanDisk(uint32_t diskIndex) {
        if (mLogger)
            mLogger->Info(FormatMessage(L"Cleaning disk {}...", diskIndex));

        auto handle = OpenDiskHandle(diskIndex);
        if (!handle) {
            return domain::Error{
                FormatMessage(L"Failed to open disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        HANDLE hDisk = Win32HandleFactory::ToWin32Handle(handle);

        CREATE_DISK createDisk{};
        createDisk.PartitionStyle = PARTITION_STYLE_GPT;
        createDisk.Gpt.MaxPartitionCount = 128;

        if (FAILED(CoCreateGuid(&createDisk.Gpt.DiskId))) {
            return domain::Error{
                FormatMessage(L"CoCreateGuid failed for disk {}", diskIndex),
                static_cast<uint32_t>(E_FAIL),
                domain::ErrorCategory::Disk
            };
        }

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDisk, IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            nullptr, 0, nullptr, 0,
            &bytesReturned, nullptr
        );

        if (!result) {
            if (mLogger) {
                mLogger->Warning(
                    L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX returned error " +
                    std::to_wstring(GetLastError()) + L", trying alternative method"
                );
            }
        }

        result = DeviceIoControl(
            hDisk, IOCTL_DISK_CREATE_DISK,
            &createDisk, sizeof(createDisk),
            nullptr, 0,
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                FormatMessage(L"IOCTL_DISK_CREATE_DISK failed for disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        result = DeviceIoControl(
            hDisk, IOCTL_DISK_UPDATE_PROPERTIES,
            nullptr, 0, nullptr, 0,
            &bytesReturned, nullptr
        );

        if (!result) {
            if (mLogger)
                mLogger->Warning(L"IOCTL_DISK_UPDATE_PROPERTIES failed, but continuing...");
        }

        if (mLogger)
            mLogger->Info(FormatMessage(L"Disk {} cleaned successfully", diskIndex));

        return domain::Expected<void>();
    }

    domain::Expected<void> Win32DiskService::CreatePartitionLayout(
        uint32_t diskIndex,
        const abstractions::PartitionLayout& layout
    ) {
        if (mLogger)
            mLogger->Info(FormatMessage(L"Creating partition layout on disk {}...", diskIndex));

        if (!layout.IsValid()) {
            return domain::Error{
                L"Invalid partition layout",
                ERROR_INVALID_PARAMETER,
                domain::ErrorCategory::Disk
            };
        }

        auto handle = OpenDiskHandle(diskIndex);
        if (!handle) {
            return domain::Error{
                FormatMessage(L"Failed to open disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        HANDLE hDisk = Win32HandleFactory::ToWin32Handle(handle);

        auto geometryResult = GetDiskGeometry(hDisk);
        if (!geometryResult.HasValue())
            return geometryResult.GetError();

        const auto& geometry = geometryResult.Value();
        uint64_t    totalSize = geometry.DiskSize.QuadPart;

        size_t layoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
            (layout.partitions.size() * sizeof(PARTITION_INFORMATION_EX));

        std::vector<BYTE> buffer(layoutBufferSize, 0);
        auto driveLayout = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX*>(buffer.data());

        if (layout.style == abstractions::PartitionLayout::Style::GPT) {
            driveLayout->PartitionStyle = PARTITION_STYLE_GPT;
            if (FAILED(CoCreateGuid(&driveLayout->Gpt.DiskId))) {
                return domain::Error{
                    FormatMessage(L"CoCreateGuid failed for disk {}", diskIndex),
                    static_cast<uint32_t>(E_FAIL),
                    domain::ErrorCategory::Disk
                };
            }
            driveLayout->Gpt.StartingUsableOffset.QuadPart = 1024 * 1024;
            driveLayout->Gpt.UsableLength.QuadPart = totalSize - (2 * 1024 * 1024);
            driveLayout->Gpt.MaxPartitionCount = 128;
        }
        else {
            driveLayout->PartitionStyle = PARTITION_STYLE_MBR;
            driveLayout->Mbr.Signature = static_cast<DWORD>(GetTickCount64());
        }

        driveLayout->PartitionCount = static_cast<DWORD>(layout.partitions.size());

        uint64_t currentOffset = (layout.style == abstractions::PartitionLayout::Style::GPT)
            ? 1024 * 1024
            : geometry.Geometry.BytesPerSector;

        for (size_t i = 0; i < layout.partitions.size(); ++i) {
            const auto& partition = layout.partitions[i];
            auto& partInfo = driveLayout->PartitionEntry[i];

            partInfo.PartitionStyle = (layout.style == abstractions::PartitionLayout::Style::GPT)
                ? PARTITION_STYLE_GPT : PARTITION_STYLE_MBR;

            partInfo.StartingOffset.QuadPart = currentOffset;
            partInfo.PartitionLength.QuadPart = partition.GetSize().ToBytes();
            partInfo.PartitionNumber = static_cast<DWORD>(i + 1);
            partInfo.RewritePartition = TRUE;

            if (layout.style == abstractions::PartitionLayout::Style::GPT) {
                if (FAILED(CoCreateGuid(&partInfo.Gpt.PartitionId))) {
                    return domain::Error{
                        FormatMessage(L"CoCreateGuid failed for partition {}", static_cast<uint32_t>(i)),
                        static_cast<uint32_t>(E_FAIL),
                        domain::ErrorCategory::Disk
                    };
                }

                if (partition.GetType() == domain::PartitionType::EFI)
                    partInfo.Gpt.PartitionType = PARTITION_SYSTEM_GUID;
                else if (partition.GetType() == domain::PartitionType::MSR)
                    partInfo.Gpt.PartitionType = PARTITION_MSFT_RESERVED_GUID;
                else
                    partInfo.Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;

                std::wstring name = partition.GetLabel();
                if (name.length() > 36)
                    name = name.substr(0, 36);
                wcsncpy_s(partInfo.Gpt.Name, 36, name.c_str(), _TRUNCATE);
                partInfo.Gpt.Attributes = 0;
            }
            else {
                partInfo.Mbr.PartitionType = 0x07;
                partInfo.Mbr.BootIndicator = (i == 0) ? TRUE : FALSE;
                partInfo.Mbr.RecognizedPartition = TRUE;
                partInfo.Mbr.HiddenSectors = 0;
            }

            currentOffset += partition.GetSize().ToBytes();
        }

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDisk, IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            driveLayout, static_cast<DWORD>(layoutBufferSize),
            nullptr, 0,
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                FormatMessage(L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX failed for disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        DeviceIoControl(
            hDisk, IOCTL_DISK_UPDATE_PROPERTIES,
            nullptr, 0, nullptr, 0,
            &bytesReturned, nullptr
        );

        if (mLogger) {
            mLogger->Info(FormatMessage(L"Created {} partitions on disk {}",
                layout.partitions.size(), diskIndex));
        }

        return domain::Expected<void>();
    }

    domain::Expected<void> Win32DiskService::FormatPartition(
        uint32_t diskIndex,
        uint32_t partitionIndex,
        domain::FileSystemType fileSystem,
        bool quickFormat
    ) {
        if (mLogger) {
            mLogger->Info(
                L"Formatting partition " + std::to_wstring(partitionIndex) +
                L" on disk " + std::to_wstring(diskIndex) + L"..."
            );
        }

        auto volumeResult = GetPartitionVolumePath(diskIndex, partitionIndex);
        if (!volumeResult.HasValue())
            return volumeResult.GetError();

        std::wstring volumePath = volumeResult.Value();

        HANDLE hVolume = CreateFileW(
            volumePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (hVolume == INVALID_HANDLE_VALUE) {
            return domain::Error{
                FormatMessage(L"Failed to open volume for partition {}", partitionIndex),
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        adapters::platform::UniqueHandle volumeHandle = Win32HandleFactory::MakeHandle(hVolume);

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hVolume, FSCTL_LOCK_VOLUME,
            nullptr, 0, nullptr, 0,
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                L"Failed to lock volume",
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        result = DeviceIoControl(
            hVolume, FSCTL_DISMOUNT_VOLUME,
            nullptr, 0, nullptr, 0,
            &bytesReturned, nullptr
        );

        if (!result) {
            DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
            return domain::Error{
                L"Failed to dismount volume",
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        std::wstring fsName;
        DWORD clusterSize = 4096;

        switch (fileSystem) {
        case domain::FileSystemType::NTFS:  fsName = L"NTFS";  clusterSize = 4096;  break;
        case domain::FileSystemType::FAT32: fsName = L"FAT32"; clusterSize = 4096;  break;
        case domain::FileSystemType::exFAT: fsName = L"EXFAT"; clusterSize = 32768; break;
        case domain::FileSystemType::ReFS:  fsName = L"REFS";  clusterSize = 65536; break;
        default:                            fsName = L"NTFS";  clusterSize = 4096;  break;
        }

        std::wstring label = L"Windows";
        BOOL         formatResult = FALSE;

        if (fileSystem == domain::FileSystemType::NTFS) {
            struct FORMAT_PARAMETERS {
                MEDIA_TYPE MediaType;
                ULONG      StartingOffset;
                ULONG      ClusterSize;
                ULONG      LabelLength;
                WCHAR      Label[11];
            };

            FORMAT_PARAMETERS formatParams{};
            formatParams.MediaType = FixedMedia;
            formatParams.StartingOffset = 0;
            formatParams.ClusterSize = clusterSize;
            formatParams.LabelLength = static_cast<ULONG>(label.length());
            wcsncpy_s(formatParams.Label, 11, label.c_str(), _TRUNCATE);

            formatResult = DeviceIoControl(
                hVolume, FSCTL_FORMAT_VOLUME,
                &formatParams, sizeof(formatParams),
                nullptr, 0,
                &bytesReturned, nullptr
            );
        }

        DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);

        if (!formatResult) {
            std::wstring cmd = L"format " + volumePath + L" /FS:" + fsName + L" /Q /Y";
            std::wstring params = L"/C " + cmd;

            SHELLEXECUTEINFOW sei{};
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
            sei.lpVerb = L"runas";
            sei.lpFile = L"cmd.exe";
            sei.lpParameters = params.c_str();
            sei.nShow = SW_HIDE;

            if (!ShellExecuteExW(&sei)) {
                return domain::Error{
                    L"Failed to format partition via cmd",
                    GetLastError(),
                    domain::ErrorCategory::Volume
                };
            }

            if (sei.hProcess) {
                WaitForSingleObject(sei.hProcess, 60000);
                CloseHandle(sei.hProcess);
            }
        }

        if (mLogger) {
            mLogger->Info(
                L"Partition " + std::to_wstring(partitionIndex) +
                L" formatted as " + fsName
            );
        }

        return domain::Expected<void>();
    }

    domain::Expected<abstractions::PartitionLayout> Win32DiskService::GetCurrentLayout(uint32_t diskIndex) {
        auto handle = OpenDiskHandle(diskIndex);
        if (!handle) {
            return domain::Error{
                FormatMessage(L"Failed to open disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        HANDLE hDisk = Win32HandleFactory::ToWin32Handle(handle);

        std::vector<BYTE> buffer(8192, 0);
        auto driveLayout = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX*>(buffer.data());

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr, 0,
            driveLayout, static_cast<DWORD>(buffer.size()),
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                FormatMessage(L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed for disk {}", diskIndex),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        abstractions::PartitionLayout layout;
        layout.style = (driveLayout->PartitionStyle == PARTITION_STYLE_GPT)
            ? abstractions::PartitionLayout::Style::GPT
            : abstractions::PartitionLayout::Style::MBR;

        for (DWORD i = 0; i < driveLayout->PartitionCount; ++i) {
            const auto& partInfo = driveLayout->PartitionEntry[i];

            if (partInfo.PartitionLength.QuadPart == 0)
                continue;

            domain::PartitionType partType = domain::PartitionType::Basic;

            if (layout.style == abstractions::PartitionLayout::Style::GPT) {
                if (AreGuidsEqual(partInfo.Gpt.PartitionType, PARTITION_SYSTEM_GUID))
                    partType = domain::PartitionType::EFI;
                else if (AreGuidsEqual(partInfo.Gpt.PartitionType, PARTITION_MSFT_RESERVED_GUID))
                    partType = domain::PartitionType::MSR;
            }

            domain::PartitionInfo partition(
                static_cast<uint32_t>(i),
                partType,
                domain::DiskSize::FromBytes(partInfo.PartitionLength.QuadPart),
                domain::FileSystemType::Unknown
            );

            if (layout.style == abstractions::PartitionLayout::Style::GPT) {
                std::wstring name(partInfo.Gpt.Name);
                partition.SetLabel(name);
            }

            layout.partitions.push_back(partition);
        }

        return layout;
    }

    domain::Expected<void> Win32DiskService::RestoreLayout(
        uint32_t diskIndex,
        const abstractions::PartitionLayout& layout
    ) {
        if (mLogger)
            mLogger->Info(FormatMessage(L"Restoring layout on disk {}...", diskIndex));

        auto cleanResult = CleanDisk(diskIndex);
        if (!cleanResult.HasValue())
            return cleanResult;

        return CreatePartitionLayout(diskIndex, layout);
    }

    domain::Expected<DISK_GEOMETRY_EX> Win32DiskService::GetDiskGeometry(HANDLE hDisk) {
        DISK_GEOMETRY_EX geometry{};
        DWORD bytesReturned = 0;

        BOOL result = DeviceIoControl(
            hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            nullptr, 0,
            &geometry, sizeof(geometry),
            &bytesReturned, nullptr
        );

        if (!result) {
            return domain::Error{
                L"Failed to get disk geometry",
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        return geometry;
    }

    domain::Expected<std::wstring> Win32DiskService::GetPartitionVolumePath(
        uint32_t diskIndex,
        uint32_t partitionIndex
    ) {
        return L"\\\\.\\PHYSICALDRIVE" + std::to_wstring(diskIndex) +
            L"\\Partition" + std::to_wstring(partitionIndex);
    }

}
