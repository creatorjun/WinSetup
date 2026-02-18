// src/application/usecases/system/AnalyzeSystemUseCase.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SystemInfo.h>
#include <abstractions/services/platform/ISystemInfoService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <memory>

namespace winsetup::application {

    class AnalyzeSystemUseCase {
    public:
        explicit AnalyzeSystemUseCase(
            std::shared_ptr<abstractions::ISystemInfoService> systemInfoService,
            std::shared_ptr<abstractions::ILogger>            logger
        );
        ~AnalyzeSystemUseCase() = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SystemInfo>> Execute();

    private:
        std::shared_ptr<abstractions::ISystemInfoService> mSystemInfoService;
        std::shared_ptr<abstractions::ILogger>            mLogger;
    };

}
