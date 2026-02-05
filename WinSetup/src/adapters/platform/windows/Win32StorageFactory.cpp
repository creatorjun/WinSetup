#include "Win32StorageFactory.h"
#include "storage/Win32DiskService.h"
#include "storage/Win32VolumeService.h"
#include "storage/Win32PartitionService.h"
#include "storage/Win32StorageScanner.h"
#include "system/Win32SystemInfoService.h"

namespace winsetup::adapters {

    Win32StorageFactory::Win32StorageFactory(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : textEncoder_(std::move(textEncoder))
        , logger_(std::move(logger)) {
    }

    std::shared_ptr<abstractions::IDiskService>
        Win32StorageFactory::CreateDiskService() const {
        return std::make_shared<Win32DiskService>(textEncoder_, logger_);
    }

    std::shared_ptr<abstractions::IVolumeService>
        Win32StorageFactory::CreateVolumeService() const {
        return std::make_shared<Win32VolumeService>(textEncoder_);
    }

    std::shared_ptr<abstractions::IPartitionService>
        Win32StorageFactory::CreatePartitionService() const {
        return std::make_shared<Win32PartitionService>(textEncoder_);
    }

    std::shared_ptr<abstractions::IStorageScanner>
        Win32StorageFactory::CreateStorageScanner(
            std::shared_ptr<abstractions::IVolumeService> volumeService
        ) const {
        return std::make_shared<Win32StorageScanner>(
            std::move(volumeService),
            textEncoder_
        );
    }

    std::shared_ptr<abstractions::ISystemInfoService>
        Win32StorageFactory::CreateSystemInfoService() const {
        return std::make_shared<Win32SystemInfoService>(textEncoder_);
    }

}
