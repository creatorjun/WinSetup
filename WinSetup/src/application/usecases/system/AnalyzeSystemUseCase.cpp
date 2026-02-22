// src/application/usecases/system/AnalyzeSystemUseCase.cpp
#include "application/usecases/system/AnalyzeSystemUseCase.h"
#include <domain/services/PathNormalizer.h>

namespace winsetup::application {

    AnalyzeSystemUseCase::AnalyzeSystemUseCase(
        std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
        std::shared_ptr<abstractions::ILogger>            logger
    )
        : mSystemInfoService(std::move(systemInfoService))
        , mDiskService(nullptr)
        , mVolumeService(nullptr)
        , mLogger(std::move(logger))
    {
    }

    AnalyzeSystemUseCase::AnalyzeSystemUseCase(
        std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
        std::shared_ptr<abstractions::IDiskService>       diskService,
        std::shared_ptr<abstractions::IVolumeService>     volumeService,
        std::shared_ptr<abstractions::ILogger>            logger
    )
        : mSystemInfoService(std::move(systemInfoService))
        , mDiskService(std::move(diskService))
        , mVolumeService(std::move(volumeService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<abstractions::SystemAnalysisResult>>
        AnalyzeSystemUseCase::Execute() {
        if (!mSystemInfoService) {
            return domain::Error(L"ISystemInfoService not provided", 0, domain::ErrorCategory::System);
        }
        if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: Analysis started.");

        auto sysResult = CollectSystemInfo();
        if (!sysResult.HasValue()) return sysResult.GetError();

        auto diskResult = CollectDisks();
        if (!diskResult.HasValue()) return diskResult.GetError();

        auto volumeResult = CollectVolumes();
        if (!volumeResult.HasValue()) return volumeResult.GetError();

        auto analysisResult = std::make_shared<abstractions::SystemAnalysisResult>(
            sysResult.Value(),
            diskResult.Value(),
            volumeResult.Value()
        );

        if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: Analysis complete.");
        return analysisResult;
    }

    domain::Expected<std::shared_ptr<domain::SystemInfo>>
        AnalyzeSystemUseCase::CollectSystemInfo() const {
        auto sysInfo = std::make_shared<domain::SystemInfo>();

        auto modelResult = mSystemInfoService->GetMotherboardModel();
        if (modelResult.HasValue()) {
            const std::wstring sanitized = domain::PathNormalizer::SanitizeFolderName(modelResult.Value());
            sysInfo->SetMotherboardModel(sanitized);
            if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: Motherboard = " + sanitized);
        }
        else {
            sysInfo->SetMotherboardModel(L"Unknown");
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get motherboard model - " + modelResult.GetError().GetMessage());
        }

        auto biosResult = mSystemInfoService->GetBIOSVersion();
        if (biosResult.HasValue()) {
            sysInfo->SetBIOSVersion(biosResult.Value());
            if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: BIOS = " + biosResult.Value());
        }
        else {
            sysInfo->SetBIOSVersion(L"Unknown");
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get BIOS version - " + biosResult.GetError().GetMessage());
        }

        auto uefiResult = mSystemInfoService->IsUEFIBoot();
        if (uefiResult.HasValue()) {
            sysInfo->SetUEFIBoot(uefiResult.Value());
            if (mLogger) mLogger->Info(std::wstring(L"AnalyzeSystemUseCase: UEFI = ") + (uefiResult.Value() ? L"true" : L"false"));
        }
        else {
            sysInfo->SetUEFIBoot(false);
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get UEFI status - " + uefiResult.GetError().GetMessage());
        }

        auto memResult = mSystemInfoService->GetTotalMemoryBytes();
        if (memResult.HasValue()) {
            sysInfo->SetTotalMemoryBytes(memResult.Value());
            if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: Memory = " + std::to_wstring(memResult.Value() / 1024 / 1024) + L" MB");
        }
        else {
            sysInfo->SetTotalMemoryBytes(0u);
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get memory size - " + memResult.GetError().GetMessage());
        }

        return sysInfo;
    }

    domain::Expected<std::shared_ptr<std::vector<domain::DiskInfo>>>
        AnalyzeSystemUseCase::CollectDisks() const {
        if (!mDiskService) {
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: IDiskService not provided, disk list will be empty.");
            return std::make_shared<std::vector<domain::DiskInfo>>();
        }

        auto result = mDiskService->EnumerateDisks();
        if (!result.HasValue()) {
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: Failed to enumerate disks - " + result.GetError().GetMessage());
            return std::make_shared<std::vector<domain::DiskInfo>>();
        }

        if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: Disks found = " + std::to_wstring(result.Value().size()));
        return std::make_shared<std::vector<domain::DiskInfo>>(std::move(result.Value()));
    }

    domain::Expected<std::shared_ptr<std::vector<domain::VolumeInfo>>>
        AnalyzeSystemUseCase::CollectVolumes() const {
        if (!mVolumeService) {
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: IVolumeService not provided, volume list will be empty.");
            return std::make_shared<std::vector<domain::VolumeInfo>>();
        }

        auto result = mVolumeService->EnumerateVolumes();
        if (!result.HasValue()) {
            if (mLogger) mLogger->Warning(L"AnalyzeSystemUseCase: Failed to enumerate volumes - " + result.GetError().GetMessage());
            return std::make_shared<std::vector<domain::VolumeInfo>>();
        }

        if (mLogger) mLogger->Info(L"AnalyzeSystemUseCase: Volumes found = " + std::to_wstring(result.Value().size()));
        return std::make_shared<std::vector<domain::VolumeInfo>>(std::move(result.Value()));
    }

}
