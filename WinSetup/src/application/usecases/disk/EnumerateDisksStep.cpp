// src/application/usecases/disk/EnumerateDisksStep.cpp
#include "application/usecases/disk/EnumerateDisksStep.h"

namespace winsetup::application {

    EnumerateDisksStep::EnumerateDisksStep(
        std::shared_ptr<abstractions::IDiskService> diskService,
        std::shared_ptr<abstractions::ILogger>      logger)
        : mDiskService(std::move(diskService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
        EnumerateDisksStep::Execute()
    {
        if (!mDiskService) {
            if (mLogger)
                mLogger->Warning(L"EnumerateDisksStep: IDiskService not provided, disk list will be empty.");
            return std::make_shared<std::vector<domain::DiskInfo>>();
        }

        auto result = mDiskService->EnumerateDisks();
        if (!result.HasValue()) {
            if (mLogger)
                mLogger->Warning(L"EnumerateDisksStep: Failed to enumerate disks - " + result.GetError().GetMessage());
            return std::make_shared<std::vector<domain::DiskInfo>>();
        }

        if (mLogger)
            mLogger->Info(L"EnumerateDisksStep: Disks found: " + std::to_wstring(result.Value().size()));

        return std::make_shared<std::vector<domain::DiskInfo>>(std::move(result.Value()));
    }

} // namespace winsetup::application
