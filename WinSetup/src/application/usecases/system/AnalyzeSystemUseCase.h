// src/application/usecases/system/AnalyzeSystemUseCase.h
#pragma once
#include <abstractions/usecases/IAnalyzeSystemUseCase.h>
#include <abstractions/services/platform/ISystemInfoService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <memory>

namespace winsetup::application {

    class AnalyzeSystemUseCase : public abstractions::IAnalyzeSystemUseCase {
    public:
        explicit AnalyzeSystemUseCase(
            std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
            std::shared_ptr<abstractions::ILogger>            logger);
        ~AnalyzeSystemUseCase() override = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SystemInfo>>
            Execute() override;

    private:
        std::shared_ptr<abstractions::ISystemInfoService> mSystemInfoService;
        std::shared_ptr<abstractions::ILogger>            mLogger;
    };

} // namespace winsetup::application
