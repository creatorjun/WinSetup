#pragma once

#include <memory>
#include "../../../abstractions/storage/IDiskService.h"
#include "../../../abstractions/storage/IVolumeService.h"
#include "../../../abstractions/storage/IPartitionService.h"
#include "../../../abstractions/storage/IStorageScanner.h"
#include "../../../abstractions/platform/ISystemInfoService.h"
#include "../../../abstractions/platform/ITextEncoder.h"

namespace winsetup::adapters {

    class Win32StorageFactory {
    public:
        explicit Win32StorageFactory(
            std::shared_ptr<abstractions::ITextEncoder> textEncoder
        );

        [[nodiscard]] std::shared_ptr<abstractions::IDiskService>
            CreateDiskService() const;

        [[nodiscard]] std::shared_ptr<abstractions::IVolumeService>
            CreateVolumeService() const;

        [[nodiscard]] std::shared_ptr<abstractions::IPartitionService>
            CreatePartitionService() const;

        [[nodiscard]] std::shared_ptr<abstractions::IStorageScanner>
            CreateStorageScanner(
                std::shared_ptr<abstractions::IVolumeService> volumeService
            ) const;

        [[nodiscard]] std::shared_ptr<abstractions::ISystemInfoService>
            CreateSystemInfoService() const;

    private:
        std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
    };

}
