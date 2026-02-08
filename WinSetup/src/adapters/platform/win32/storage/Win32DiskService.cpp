// src/adapters/platform/win32/storage/Win32DiskService.cpp
#include "Win32DiskService.h"
#include <adapters/platform/win32/core/Win32ErrorHandler.h>
#include <adapters/platform/win32/core/Win32HandleFactory.h>

#include <Windows.h>
#include <winioctl.h>
#include <sstream>

namespace winsetup::adapters::platform {

    Win32DiskService::Win32DiskService(std::shared_ptr<abstractions::ILogger> logger)
        : m_logger(std::move(logger))
    {
        if (m_logger) {
            m_logger->Info(L"Win32DiskService initialized");
        }
    }

    domain::UniqueHandle Win32DiskService::OpenDiskHandle(uint32_t diskIndex) {
        std::wstringstream ss;
        ss << L"\\\\.\\PhysicalDrive" << diskIndex;
        std::wstring path = ss.str();

        HANDLE hDisk = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hDisk == INVALID_HANDLE_VALUE) {
            return domain::UniqueHandle();
        }

        return Win32HandleFactory::MakeHandle(hDisk);
    }

    domain::Expected<std::vector<domain::DiskInfo>> Win32DiskService::EnumerateDisks() {
        if (m_logger) {
            m_logger->Debug(L"Enumerating disks...");
        }

        std::vector<domain::DiskInfo> disks;

        for (uint32_t i = 0; i < 32; ++i) {
            auto handle = OpenDiskHandle(i);
            if (!handle) {
                continue;
            }

            auto diskInfoResult = GetDiskInfo(i);
            if (diskInfoResult.HasValue()) {
                disks.push_back(std::move(diskInfoResult.Value()));
            }
        }

        if (m_logger) {
            std::wstringstream ss;
            ss << L"Found " << disks.size() << L" disks";
            m_logger->Info(ss.str());
        }

        return disks;
    }

    domain::Expected<domain::DiskInfo> Win32DiskService::GetDiskInfo(uint32_t diskIndex) {
        auto handle = OpenDiskHandle(diskIndex);
        if (!handle) {
            std::wstringstream ss;
            ss << L"Failed to open disk " << diskIndex;
            return domain::Error{
                ss.str(),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        DISK_GEOMETRY_EX geometry = {};
        DWORD bytesReturned = 0;

        BOOL result = DeviceIoControl(
            Win32HandleFactory::ToWin32Handle(handle),
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            nullptr,
            0,
            &geometry,
            sizeof(geometry),
            &bytesReturned,
            nullptr
        );

        if (!result) {
            std::wstringstream ss;
            ss << L"Failed to get geometry for disk " << diskIndex;
            return domain::Error{
                ss.str(),
                GetLastError(),
                domain::ErrorCategory::Disk
            };
        }

        domain::DiskInfo diskInfo;
        diskInfo.SetIndex(diskIndex);
        diskInfo.SetSize(domain::DiskSize::FromBytes(geometry.DiskSize.QuadPart));
        diskInfo.SetType(L"HDD");

        return diskInfo;
    }

    domain::Expected<void> Win32DiskService::CleanDisk(uint32_t diskIndex) {
        if (m_logger) {
            std::wstringstream ss;
            ss << L"Cleaning disk " << diskIndex << L"...";
            m_logger->Info(ss.str());
        }

        return domain::Expected<void>();
    }

    domain::Expected<void> Win32DiskService::CreatePartitionLayout(
        uint32_t diskIndex,
        const abstractions::PartitionLayout& layout)
    {
        if (m_logger) {
            std::wstringstream ss;
            ss << L"Creating partition layout on disk " << diskIndex << L"...";
            m_logger->Info(ss.str());
        }

        return domain::Expected<void>();
    }

    domain::Expected<void> Win32DiskService::FormatPartition(
        uint32_t diskIndex,
        uint32_t partitionIndex,
        domain::FileSystemType fileSystem,
        bool quickFormat)
    {
        if (m_logger) {
            std::wstringstream ss;
            ss << L"Formatting partition " << partitionIndex
                << L" on disk " << diskIndex << L"...";
            m_logger->Info(ss.str());
        }

        return domain::Expected<void>();
    }

    domain::Expected<abstractions::PartitionLayout> Win32DiskService::GetCurrentLayout(
        uint32_t diskIndex)
    {
        abstractions::PartitionLayout layout;
        layout.style = abstractions::PartitionLayout::Style::GPT;

        return layout;
    }

    domain::Expected<void> Win32DiskService::RestoreLayout(
        uint32_t diskIndex,
        const abstractions::PartitionLayout& layout)
    {
        if (m_logger) {
            std::wstringstream ss;
            ss << L"Restoring layout on disk " << diskIndex << L"...";
            m_logger->Info(ss.str());
        }

        return domain::Expected<void>();
    }

}
