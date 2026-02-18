// src/application/usecases/system/AnalyzeSystemUseCase.cpp
#include <application/usecases/system/AnalyzeSystemUseCase.h>
#include <domain/services/PathNormalizer.h>

namespace winsetup::application {

    AnalyzeSystemUseCase::AnalyzeSystemUseCase(
        std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
        std::shared_ptr<abstractions::ILogger>            logger)
        : mSystemInfoService(std::move(systemInfoService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<domain::SystemInfo>> AnalyzeSystemUseCase::Execute() {
        if (!mSystemInfoService)
            return domain::Error(
                L"ISystemInfoService not provided",
                static_cast<uint32_t>(0),
                domain::ErrorCategory::System);

        if (mLogger)
            mLogger->Info(L"AnalyzeSystemUseCase: Analyzing system information...");

        auto sysInfo = std::make_shared<domain::SystemInfo>();

        auto modelResult = mSystemInfoService->GetMotherboardModel();
        if (modelResult.HasValue()) {
            const std::wstring raw = modelResult.Value();
            const std::wstring sanitized = domain::PathNormalizer::SanitizeFolderName(raw);
            sysInfo->SetMotherboardModel(sanitized);
            if (mLogger) {
                mLogger->Info(L"AnalyzeSystemUseCase: Motherboard raw       = " + raw);
                mLogger->Info(L"AnalyzeSystemUseCase: Motherboard sanitized = " + sanitized);
            }
        }
        else {
            sysInfo->SetMotherboardModel(L"Unknown");
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get motherboard model: "
                    + modelResult.GetError().GetMessage());
        }

        auto biosResult = mSystemInfoService->GetBIOSVersion();
        if (biosResult.HasValue()) {
            sysInfo->SetBIOSVersion(biosResult.Value());
            if (mLogger)
                mLogger->Info(L"AnalyzeSystemUseCase: BIOS version = " + biosResult.Value());
        }
        else {
            sysInfo->SetBIOSVersion(L"Unknown");
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get BIOS version: "
                    + biosResult.GetError().GetMessage());
        }

        auto uefiResult = mSystemInfoService->IsUEFIBoot();
        if (uefiResult.HasValue()) {
            sysInfo->SetUEFIBoot(uefiResult.Value());
            if (mLogger)
                mLogger->Info(std::wstring(L"AnalyzeSystemUseCase: UEFI boot = ")
                    + (uefiResult.Value() ? L"true" : L"false"));
        }
        else {
            sysInfo->SetUEFIBoot(false);
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get UEFI boot status: "
                    + uefiResult.GetError().GetMessage());
        }

        auto memResult = mSystemInfoService->GetTotalMemoryBytes();
        if (memResult.HasValue()) {
            sysInfo->SetTotalMemoryBytes(memResult.Value());
            if (mLogger)
                mLogger->Info(L"AnalyzeSystemUseCase: Total memory = "
                    + std::to_wstring(memResult.Value() / (1024 * 1024)) + L" MB");
        }
        else {
            sysInfo->SetTotalMemoryBytes(0u);
            if (mLogger)
                mLogger->Warning(L"AnalyzeSystemUseCase: Failed to get memory size: "
                    + memResult.GetError().GetMessage());
        }

        if (mLogger)
            mLogger->Info(L"AnalyzeSystemUseCase: System analysis complete.");

        return sysInfo;
    }

} // namespace winsetup::application
