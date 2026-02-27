// src/application/usecases/disk/EnumerateDisksStep.h
#pragma once
#include "abstractions/usecases/steps/IEnumerateDisksStep.h"
#include "abstractions/services/storage/IDiskService.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "domain/primitives/Expected.h"
#include "domain/entities/DiskInfo.h"
#include <memory>
#include <vector>

namespace winsetup::application {

    class EnumerateDisksStep final : public abstractions::IEnumerateDisksStep {
    public:
        explicit EnumerateDisksStep(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::ILogger>      logger);

        ~EnumerateDisksStep() override = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
            Execute() override;

    private:
        std::shared_ptr<abstractions::IDiskService> mDiskService;
        std::shared_ptr<abstractions::ILogger>      mLogger;
    };

} // namespace winsetup::application
