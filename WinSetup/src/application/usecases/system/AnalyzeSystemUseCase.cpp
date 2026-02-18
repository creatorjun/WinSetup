// src/application/usecases/system/AnalyzeSystemUseCase.cpp
#include "AnalyzeSystemUseCase.h"
#include <domain/services/PathNormalizer.h>

namespace winsetup::application {

    AnalyzeSystemUseCase::AnalyzeSystemUseCase(
        std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
        std::shared_ptr<abstractions::ILogger>            logger
    )
        : mSystemInfoService(std::move(systemInfoService))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<std::shared_ptr<domain::SystemInfo>> AnalyzeSystemUseCase::Execute() {
        if (mLogger) {
            mLogger->Info(L"[AnalyzeSystemUseCase] Analyzing system information...");
        }

        auto sysInfo = std::make_shared<domain::SystemInfo>();

        auto modelResult = mSystemInfoService->GetMotherboardModel();
        if (modelResult.HasValue()) {
            const std::wstring raw = modelResult.Value();
            const std::wstring sanitized = domain::PathNormalizer::SanitizeFolderName(raw);
            sysInfo->SetMotherboardModel(sanitized);
            if (mLogger) {
                mLogger->Info(L"[AnalyzeSystemUseCase] Motherboard (raw)       : " + raw);
                mLogger->Info(L"[AnalyzeSystemUseCase] Motherboard (sanitized) : " + sanitized);
            }
        }
        else {
            if (mLogger) {
                mLogger->Warning(L"[AnalyzeSystemUseCase] Failed to get motherboard model: "
                    + modelResult.GetError().GetMessage());
            }
            sysInfo->SetMotherboardModel(L"Unknown");
        }

        auto biosResult = mSystemInfoService->GetBIOSVersion();
        if (biosResult.HasValue()) {
            sysInfo->SetBIOSVersion(biosResult.Value());
        }
        else {
            sysInfo->SetBIOSVersion(L"Unknown");
        }

        auto uefiResult = mSystemInfoService->IsUEFIBoot();
        if (uefiResult.HasValue()) {
            sysInfo->SetUEFIBoot(uefiResult.Value());
        }

        auto memResult = mSystemInfoService->GetTotalMemoryBytes();
        if (memResult.HasValue()) {
            sysInfo->SetTotalMemoryBytes(memResult.Value());
        }

        if (mLogger) {
            mLogger->Info(L"[AnalyzeSystemUseCase] System analysis complete.");
        }

        return sysInfo;
    }

}
