#include <application/usecases/disk/EnumerateDisksUseCase.h>

namespace winsetup::application {

    EnumerateDisksUseCase::EnumerateDisksUseCase(
        std::shared_ptr<abstractions::IDiskService> diskService,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mDiskService(std::move(diskService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
        EnumerateDisksUseCase::Execute()
    {
        if (!mDiskService) {
            if (mLogger)
                mLogger->Warning(L"EnumerateDisksUseCase: IDiskService not provided, disk list will be empty.");
            return std::make_shared<std::vector<domain::DiskInfo>>();
        }

        auto result = mDiskService->EnumerateDisks();
        if (!result.HasValue()) {
            if (mLogger)
                mLogger->Warning(L"EnumerateDisksUseCase: Failed to enumerate disks - " + result.GetError().GetMessage());
            return std::make_shared<std::vector<domain::DiskInfo>>();
        }

        if (mLogger)
            mLogger->Info(L"EnumerateDisksUseCase: Disks found: " + std::to_wstring(result.Value().size()));

        return std::make_shared<std::vector<domain::DiskInfo>>(std::move(result.Value()));
    }

}
