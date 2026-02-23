// src/application/usecases/disk/EnumerateDisksUseCase.h
#pragma once

#include <abstractions/usecases/IEnumerateDisksUseCase.h>
#include <abstractions/services/storage/IDiskService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/DiskInfo.h>
#include <memory>
#include <vector>

namespace winsetup::application {

    class EnumerateDisksUseCase final : public abstractions::IEnumerateDisksUseCase {
    public:
        explicit EnumerateDisksUseCase(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::ILogger> logger
        );
        ~EnumerateDisksUseCase() override = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
            Execute() override;

    private:
        std::shared_ptr<abstractions::IDiskService> mDiskService;
        std::shared_ptr<abstractions::ILogger>      mLogger;
    };

}
