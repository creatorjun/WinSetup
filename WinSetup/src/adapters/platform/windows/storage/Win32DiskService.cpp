#include "Win32DiskService.h"
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <winioctl.h>
#include <algorithm>
#include <chrono>

namespace winsetup::adapters {

    Win32DiskService::Win32DiskService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : textEncoder_(std::move(textEncoder))
        , logger_(std::move(logger)) {

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Win32DiskService initialized",
                L"DiskService"
            );
        }
    }

    domain::Expected<std::vector<domain::DiskInfo>>
        Win32DiskService::EnumeratePhysicalDisks() const noexcept {

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Starting physical disk enumeration",
                L"DiskService"
            );
        }

        std::vector<domain::DiskInfo> disks;
        disks.reserve(16);

        for (uint32_t i = 0; i < 256; ++i) {
            domain::PhysicalDiskId diskId{ i };
            auto diskInfoResult = GetDiskInfo(diskId);

            if (diskInfoResult.HasValue()) [[likely]] {
                disks.push_back(std::move(diskInfoResult).Value());
            }
        }

        if (disks.empty()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"No physical disks found in system",
                    L"DiskService"
                );
            }
            return domain::Expected<std::vector<domain::DiskInfo>>::Failure(
                domain::Error("No physical disks found")
            );
        }

        if (logger_) {
            std::wstring message = L"Found " + std::to_wstring(disks.size()) + L" physical disk(s)";
            logger_->Log(
                abstractions::LogLevel::Info,
                message,
                L"DiskService"
            );
        }

        return domain::Expected<std::vector<domain::DiskInfo>>::Success(
            std::move(disks)
        );
    }

    domain::Expected<domain::DiskInfo>
        Win32DiskService::GetDiskInfo(domain::PhysicalDiskId diskId) const noexcept {

        if (logger_) {
            std::wstring message = L"Querying disk info for PhysicalDrive" + std::to_wstring(diskId.index);
            logger_->Log(
                abstractions::LogLevel::Debug,
                message,
                L"DiskService"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ);
        if (handleResult.HasError()) [[unlikely]] {
            return domain::Expected<domain::DiskInfo>::Failure(
                std::move(handleResult).GetError()
            );
        }

        HANDLE diskHandle = handleResult.Value();
        auto closeGuard = [diskHandle]() { CloseHandle(diskHandle); };

        auto busTypeResult = QueryBusType(diskHandle);
        if (busTypeResult.HasError()) [[unlikely]] {
            closeGuard();
            return domain::Expected<domain::DiskInfo>::Failure(
                std::move(busTypeResult).GetError()
            );
        }

        auto sizeResult = QueryDiskSize(diskHandle);
        if (sizeResult.HasError()) [[unlikely]] {
            closeGuard();
            return domain::Expected<domain::DiskInfo>::Failure(
                std::move(sizeResult).GetError()
            );
        }

        auto modelResult = QueryDiskModel(diskHandle);
        std::wstring model = modelResult.HasValue()
            ? std::move(modelResult).Value()
            : L"Unknown";

        domain::DiskInfo diskInfo(
            diskId,
            busTypeResult.Value(),
            sizeResult.Value(),
            std::move(model)
        );

        auto styleResult = QueryPartitionStyle(diskHandle);
        if (styleResult.HasValue()) [[likely]] {
            diskInfo.SetPartitionStyle(styleResult.Value());
        }

        closeGuard();

        if (logger_) {
            std::wstring message = L"PhysicalDrive" + std::to_wstring(diskId.index) +
                L": " + diskInfo.GetModel() +
                L" (" + std::to_wstring(diskInfo.GetSizeInGB()) + L" GB, " +
                domain::BusTypeToString(diskInfo.GetBusType()) + L")";
            logger_->Log(
                abstractions::LogLevel::Debug,
                message,
                L"DiskService"
            );
        }

        return domain::Expected<domain::DiskInfo>::Success(std::move(diskInfo));
    }

    domain::Expected<std::vector<domain::PartitionInfo>>
        Win32DiskService::GetPartitions(domain::PhysicalDiskId diskId) const noexcept {

        if (logger_) {
            std::wstring message = L"Querying partitions for PhysicalDrive" + std::to_wstring(diskId.index);
            logger_->Log(
                abstractions::LogLevel::Debug,
                message,
                L"DiskService"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ);
        if (handleResult.HasError()) [[unlikely]] {
            return domain::Expected<std::vector<domain::PartitionInfo>>::Failure(
                std::move(handleResult).GetError()
            );
        }

        HANDLE diskHandle = handleResult.Value();

        DRIVE_LAYOUT_INFORMATION_EX layoutInfo{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr,
            0,
            &layoutInfo,
            sizeof(layoutInfo),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            CloseHandle(diskHandle);
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to get drive layout",
                    L"DiskService"
                );
            }
            return domain::Expected<std::vector<domain::PartitionInfo>>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
            );
        }

        std::vector<domain::PartitionInfo> partitions;
        partitions.reserve(layoutInfo.PartitionCount);

        for (DWORD i = 0; i < layoutInfo.PartitionCount; ++i) {
            const auto& part = layoutInfo.PartitionEntry[i];

            if (part.PartitionLength.QuadPart == 0) {
                continue;
            }

            domain::PartitionId partId{ diskId.index, part.PartitionNumber };

            domain::PartitionInfo partInfo(
                partId,
                part.StartingOffset.QuadPart,
                part.PartitionLength.QuadPart,
                domain::PartitionType::Basic
            );

            partitions.push_back(std::move(partInfo));
        }

        CloseHandle(diskHandle);

        if (logger_) {
            std::wstring message = L"Found " + std::to_wstring(partitions.size()) +
                L" partition(s) on PhysicalDrive" + std::to_wstring(diskId.index);
            logger_->Log(
                abstractions::LogLevel::Debug,
                message,
                L"DiskService"
            );
        }

        return domain::Expected<std::vector<domain::PartitionInfo>>::Success(
            std::move(partitions)
        );
    }

    domain::Result<> Win32DiskService::CleanDisk(domain::PhysicalDiskId diskId) noexcept {
        if (logger_) {
            std::wstring message = L"[CRITICAL] Cleaning PhysicalDrive" + std::to_wstring(diskId.index);
            logger_->Log(
                abstractions::LogLevel::Warning,
                message,
                L"DiskService"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);
        if (handleResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk for cleaning",
                    L"DiskService"
                );
            }
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

        CREATE_DISK createDisk{};
        createDisk.PartitionStyle = PARTITION_STYLE_RAW;

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_CREATE_DISK,
            &createDisk,
            sizeof(createDisk),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Disk clean operation failed",
                    L"DiskService"
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_CREATE_DISK")
            );
        }

        if (logger_) {
            std::wstring message = L"Successfully cleaned PhysicalDrive" + std::to_wstring(diskId.index);
            logger_->Log(
                abstractions::LogLevel::Info,
                message,
                L"DiskService"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32DiskService::InitializeDisk(
        domain::PhysicalDiskId diskId,
        domain::PartitionStyle style
    ) noexcept {
        if (logger_) {
            std::wstring styleStr = (style == domain::PartitionStyle::GPT) ? L"GPT" : L"MBR";
            std::wstring message = L"[CRITICAL] Initializing PhysicalDrive" +
                std::to_wstring(diskId.index) + L" as " + styleStr;
            logger_->Log(
                abstractions::LogLevel::Warning,
                message,
                L"DiskService"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);
        if (handleResult.HasError()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to open disk for initialization",
                    L"DiskService"
                );
            }
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

        CREATE_DISK createDisk{};

        if (style == domain::PartitionStyle::GPT) {
            createDisk.PartitionStyle = PARTITION_STYLE_GPT;
            CoCreateGuid(&createDisk.Gpt.DiskId);
            createDisk.Gpt.MaxPartitionCount = 128;
        }
        else {
            createDisk.PartitionStyle = PARTITION_STYLE_MBR;
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            createDisk.Mbr.Signature = static_cast<ULONG>(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                );
        }

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_CREATE_DISK,
            &createDisk,
            sizeof(createDisk),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Disk initialization failed",
                    L"DiskService"
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_CREATE_DISK")
            );
        }

        if (logger_) {
            std::wstring message = L"Successfully initialized PhysicalDrive" + std::to_wstring(diskId.index);
            logger_->Log(
                abstractions::LogLevel::Info,
                message,
                L"DiskService"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32DiskService::SetDiskOnline(
        domain::PhysicalDiskId diskId
    ) noexcept {
        if (logger_) {
            std::wstring message = L"Setting PhysicalDrive" + std::to_wstring(diskId.index) + L" online";
            logger_->Log(
                abstractions::LogLevel::Info,
                message,
                L"DiskService"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);
        if (handleResult.HasError()) [[unlikely]] {
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

        SET_DISK_ATTRIBUTES attributes{};
        attributes.Version = sizeof(SET_DISK_ATTRIBUTES);
        attributes.Persist = TRUE;
        attributes.AttributesMask = DISK_ATTRIBUTE_OFFLINE;
        attributes.Attributes = 0;

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_SET_DISK_ATTRIBUTES,
            &attributes,
            sizeof(attributes),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to set disk online",
                    L"DiskService"
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DISK_ATTRIBUTES")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Disk successfully set online",
                L"DiskService"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32DiskService::SetDiskOffline(
        domain::PhysicalDiskId diskId
    ) noexcept {
        if (logger_) {
            std::wstring message = L"Setting PhysicalDrive" + std::to_wstring(diskId.index) + L" offline";
            logger_->Log(
                abstractions::LogLevel::Info,
                message,
                L"DiskService"
            );
        }

        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ | GENERIC_WRITE);
        if (handleResult.HasError()) [[unlikely]] {
            return domain::Result<>::Failure(std::move(handleResult).GetError());
        }

        HANDLE diskHandle = handleResult.Value();

        SET_DISK_ATTRIBUTES attributes{};
        attributes.Version = sizeof(SET_DISK_ATTRIBUTES);
        attributes.Persist = TRUE;
        attributes.AttributesMask = DISK_ATTRIBUTE_OFFLINE;
        attributes.Attributes = DISK_ATTRIBUTE_OFFLINE;

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_SET_DISK_ATTRIBUTES,
            &attributes,
            sizeof(attributes),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Failed to set disk offline",
                    L"DiskService"
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_SET_DISK_ATTRIBUTES")
            );
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Disk successfully set offline",
                L"DiskService"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Expected<bool> Win32DiskService::IsDiskOnline(
        domain::PhysicalDiskId diskId
    ) const noexcept {
        auto handleResult = OpenDiskHandle(diskId, GENERIC_READ);
        if (handleResult.HasError()) [[unlikely]] {
            return domain::Expected<bool>::Failure(
                std::move(handleResult).GetError()
            );
        }

        HANDLE diskHandle = handleResult.Value();

        GET_DISK_ATTRIBUTES attributes{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DISK_ATTRIBUTES,
            nullptr,
            0,
            &attributes,
            sizeof(attributes),
            &bytesReturned,
            nullptr
        );

        CloseHandle(diskHandle);

        if (!success) [[unlikely]] {
            return domain::Expected<bool>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DISK_ATTRIBUTES")
            );
        }

        bool isOnline = (attributes.Attributes & DISK_ATTRIBUTE_OFFLINE) == 0;
        return domain::Expected<bool>::Success(isOnline);
    }

    domain::Expected<HANDLE> Win32DiskService::OpenDiskHandle(
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
            if (logger_) {
                std::wstring message = L"Failed to open " + diskPath;
                logger_->Log(
                    abstractions::LogLevel::Error,
                    message,
                    L"DiskService"
                );
            }
            return domain::Expected<HANDLE>::Failure(
                CreateErrorFromLastError("CreateFileW")
            );
        }

        return domain::Expected<HANDLE>::Success(handle);
    }

    domain::Expected<domain::BusType> Win32DiskService::QueryBusType(
        HANDLE diskHandle
    ) const noexcept {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        STORAGE_DEVICE_DESCRIPTOR descriptor{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            &descriptor,
            sizeof(descriptor),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            return domain::Expected<domain::BusType>::Failure(
                CreateErrorFromLastError("IOCTL_STORAGE_QUERY_PROPERTY")
            );
        }

        return domain::Expected<domain::BusType>::Success(
            static_cast<domain::BusType>(descriptor.BusType)
        );
    }

    domain::Expected<uint64_t> Win32DiskService::QueryDiskSize(
        HANDLE diskHandle
    ) const noexcept {
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
            return domain::Expected<uint64_t>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_LENGTH_INFO")
            );
        }

        return domain::Expected<uint64_t>::Success(
            static_cast<uint64_t>(lengthInfo.Length.QuadPart)
        );
    }

    domain::Expected<std::wstring> Win32DiskService::QueryDiskModel(
        HANDLE diskHandle
    ) const noexcept {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        BYTE buffer[4096]{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            buffer,
            sizeof(buffer),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            return domain::Expected<std::wstring>::Failure(
                CreateErrorFromLastError("IOCTL_STORAGE_QUERY_PROPERTY")
            );
        }

        auto* descriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);

        if (descriptor->ProductIdOffset == 0) {
            return domain::Expected<std::wstring>::Success(L"Unknown");
        }

        const char* productId = reinterpret_cast<const char*>(
            buffer + descriptor->ProductIdOffset
            );

        auto wideResult = textEncoder_->ToWide(productId);
        if (wideResult.HasError()) [[unlikely]] {
            return domain::Expected<std::wstring>::Success(L"Unknown");
        }

        std::wstring model = std::move(wideResult).Value();
        model.erase(model.find_last_not_of(L" \t\r\n") + 1);

        return domain::Expected<std::wstring>::Success(std::move(model));
    }

    domain::Expected<domain::PartitionStyle> Win32DiskService::QueryPartitionStyle(
        HANDLE diskHandle
    ) const noexcept {
        DRIVE_LAYOUT_INFORMATION_EX layoutInfo{};
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            diskHandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            nullptr,
            0,
            &layoutInfo,
            sizeof(layoutInfo),
            &bytesReturned,
            nullptr
        );

        if (!success) [[unlikely]] {
            return domain::Expected<domain::PartitionStyle>::Failure(
                CreateErrorFromLastError("IOCTL_DISK_GET_DRIVE_LAYOUT_EX")
            );
        }

        domain::PartitionStyle style = domain::PartitionStyle::Raw;

        if (layoutInfo.PartitionStyle == PARTITION_STYLE_GPT) {
            style = domain::PartitionStyle::GPT;
        }
        else if (layoutInfo.PartitionStyle == PARTITION_STYLE_MBR) {
            style = domain::PartitionStyle::MBR;
        }

        return domain::Expected<domain::PartitionStyle>::Success(style);
    }

    domain::Error Win32DiskService::CreateErrorFromLastError(
        std::string_view operation
    ) const noexcept {
        const DWORD errorCode = ::GetLastError();

        std::string message = "Win32DiskService: ";
        message.append(operation);
        message.append(" failed with error code: ");
        message.append(std::to_string(errorCode));

        return domain::Error(std::move(message), static_cast<int>(errorCode));
    }

    std::unique_ptr<abstractions::IDiskService> CreateDiskService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    ) {
        return std::make_unique<Win32DiskService>(std::move(textEncoder), std::move(logger));
    }

}
