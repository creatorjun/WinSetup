// src/application/usecases/system/AnalyzeSystemUseCase.cpp
#include <application/usecases/system/AnalyzeSystemUseCase.h>
#include <domain/services/PathNormalizer.h>

namespace winsetup::application {

    AnalyzeSystemUseCase::AnalyzeSystemUseCase(
        std::shared_ptr<abstractions::ISystemInfoService>       systemInfoService,
        std::shared_ptr<abstractions::IEnumerateDisksUseCase>   enumerateDisks,
        std::shared_ptr<abstractions::IEnumerateVolumesUseCase> enumerateVolumes,
        std::shared_ptr<abstractions::IAnalysisRepository>      analysisRepository,
        std::shared_ptr<abstractions::ILogger>                  logger
    )
        : mSystemInfoService(std::move(systemInfoService))
        , mEnumerateDisks(std::move(enumerateDisks))
        , mEnumerateVolumes(std::move(enumerateVolumes))
        , mAnalysisRepository(std::move(analysisRepository))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<void> AnalyzeSystemUseCase::Execute()
    {
        if (!mSystemInfoService)
            return domain::Error(L"ISystemInfoService not provided", 0, domain::ErrorCategory::System);
        if (!mAnalysisRepository)
            return domain::Error(L"IAnalysisRepository not provided", 0, domain::ErrorCategory::System);

        if (mLogger)
            mLogger->Info(L"AnalyzeSystemUseCase: Analysis started.");

        auto sysResult = CollectSystemInfo();
        if (!sysResult.HasValue())
            return sysResult.GetError();

        auto diskResult = mEnumerateDisks->Execute();
        if (!diskResult.HasValue())
            return diskResult.GetError();

        auto volumeResult = mEnumerateVolumes->Execute();
        if (!volumeResult.HasValue())
            return volumeResult.GetError();

        mAnalysisRepository->StoreSystemInfo(sysResult.Value());
        mAnalysisRepository->StoreDisks(diskResult.Value());
        mAnalysisRepository->StoreVolumes(volumeResult.Value());

        if (mLogger)
            mLogger->Info(L"AnalyzeSystemUseCase: Analysis complete. Stored to repository.");

        LogStoredResults(*diskResult.Value(), *volumeResult.Value());

        return domain::Expected<void>();
    }

    domain::Expected<std::shared_ptr<domain::SystemInfo>>
        AnalyzeSystemUseCase::CollectSystemInfo() const
    {
        auto sysInfo = std::make_shared<domain::SystemInfo>();

        auto modelResult = mSystemInfoService->GetMotherboardModel();
        if (modelResult.HasValue()) {
            const std::wstring sanitized = domain::PathNormalizer::SanitizeFolderName(modelResult.Value());
            sysInfo->SetMotherboardModel(sanitized);
            if (mLogger)
                mLogger->Info(L"AnalyzeSystemUseCase: Motherboard: " + sanitized);
        }
        else {
            sysInfo->SetMotherboardModel(L"Unknown");
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get motherboard model - " + modelResult.GetError().GetMessage());
        }

        auto biosResult = mSystemInfoService->GetBIOSVersion();
        if (biosResult.HasValue()) {
            sysInfo->SetBIOSVersion(biosResult.Value());
            if (mLogger)
                mLogger->Info(L"AnalyzeSystemUseCase: BIOS: " + biosResult.Value());
        }
        else {
            sysInfo->SetBIOSVersion(L"Unknown");
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get BIOS version - " + biosResult.GetError().GetMessage());
        }

        auto uefiResult = mSystemInfoService->IsUEFIBoot();
        if (uefiResult.HasValue()) {
            sysInfo->SetUEFIBoot(uefiResult.Value());
            if (mLogger)
                mLogger->Info(std::wstring(L"AnalyzeSystemUseCase: UEFI: ") + (uefiResult.Value() ? L"true" : L"false"));
        }
        else {
            sysInfo->SetUEFIBoot(false);
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get UEFI status - " + uefiResult.GetError().GetMessage());
        }

        auto memResult = mSystemInfoService->GetTotalMemoryBytes();
        if (memResult.HasValue()) {
            sysInfo->SetTotalMemoryBytes(memResult.Value());
            if (mLogger)
                mLogger->Info(L"AnalyzeSystemUseCase: Memory: " + std::to_wstring(memResult.Value() / 1024 / 1024) + L" MB");
        }
        else {
            sysInfo->SetTotalMemoryBytes(0u);
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get memory size - " + memResult.GetError().GetMessage());
        }

        return sysInfo;
    }

    void AnalyzeSystemUseCase::LogStoredResults(
        const std::vector<domain::DiskInfo>& disks,
        const std::vector<domain::VolumeInfo>& volumes
    ) const
    {
        if (!mLogger) return;

        mLogger->Info(L"AnalyzeSystemUseCase: ---- Disk Summary (" + std::to_wstring(disks.size()) + L") ----");
        for (const auto& disk : disks) {
            const std::wstring sizeStr = std::to_wstring(static_cast<uint64_t>(disk.GetSize().ToGB())) + L" GB";
            const std::wstring typeStr = disk.IsSSD() ? L"SSD" : (disk.IsNVMe() ? L"NVMe" : L"HDD");
            const std::wstring busStr = BusTypeToString(disk.GetBusType());
            const std::wstring partStr = std::to_wstring(disk.GetPartitions().size()) + L" partition(s)";
            const std::wstring roleStr = disk.IsSystem() ? L"[System]" : (disk.IsData() ? L"[Data]" : L"");
            mLogger->Info(
                L"  Disk[" + std::to_wstring(disk.GetIndex()) + L"] " +
                sizeStr + L" / " + typeStr + L" / " + busStr + L" / " + partStr +
                (roleStr.empty() ? L"" : L" " + roleStr)
            );
        }

        mLogger->Info(L"AnalyzeSystemUseCase: ---- Volume Summary (" + std::to_wstring(volumes.size()) + L") ----");
        for (const auto& vol : volumes) {
            const std::wstring sizeStr = std::to_wstring(static_cast<uint64_t>(vol.GetSize().ToGB())) + L" GB";
            const std::wstring fsStr = FileSystemTypeToString(vol.GetFileSystem());
            const std::wstring mounted = vol.IsMounted() ? L"Mounted" : L"Unmounted";
            const std::wstring letter = vol.GetLetter().empty() ? L"-" : vol.GetLetter();
            const std::wstring roleStr = vol.IsSystem() ? L"[System]"
                : vol.IsData() ? L"[Data]"
                : vol.IsBoot() ? L"[Boot]"
                : L"";
            mLogger->Info(
                L"  Vol[" + letter + L"] " +
                L"\"" + vol.GetLabel() + L"\" " +
                sizeStr + L" / " + fsStr + L" / " + mounted + L" / " + vol.GetVolumePath() +
                (roleStr.empty() ? L"" : L" " + roleStr)
            );
        }
    }

    std::wstring AnalyzeSystemUseCase::BusTypeToString(domain::BusType busType)
    {
        switch (busType) {
        case domain::BusType::SATA: return L"SATA";
        case domain::BusType::NVME: return L"NVMe";
        case domain::BusType::USB:  return L"USB";
        case domain::BusType::SCSI: return L"SCSI";
        default:                    return L"Unknown";
        }
    }

    std::wstring AnalyzeSystemUseCase::FileSystemTypeToString(domain::FileSystemType fs)
    {
        switch (fs) {
        case domain::FileSystemType::NTFS:  return L"NTFS";
        case domain::FileSystemType::FAT32: return L"FAT32";
        case domain::FileSystemType::exFAT: return L"exFAT";
        case domain::FileSystemType::ReFS:  return L"ReFS";
        default:                            return L"Unknown";
        }
    }

}
