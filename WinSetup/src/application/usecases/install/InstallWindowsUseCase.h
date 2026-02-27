// src\application\usecases\install\InstallWindowsUseCase.h
#pragma once
#include "abstractions/usecases/IInstallWindowsUseCase.h"
#include "abstractions/usecases/IApplyImageUseCase.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include <memory>

namespace winsetup::application {

    class InstallWindowsUseCase final : public abstractions::IInstallWindowsUseCase {
    public:
        explicit InstallWindowsUseCase(
            std::shared_ptr<abstractions::IApplyImageUseCase> applyImageUseCase,
            std::shared_ptr<abstractions::ILogger>            logger);
        ~InstallWindowsUseCase() override = default;
        InstallWindowsUseCase(const InstallWindowsUseCase&) = delete;
        InstallWindowsUseCase& operator=(const InstallWindowsUseCase&) = delete;

        [[nodiscard]] domain::Expected<void> Execute(
            std::shared_ptr<const domain::SetupConfig> config) override;

    private:
        std::shared_ptr<abstractions::IApplyImageUseCase> mApplyImageUseCase;
        std::shared_ptr<abstractions::ILogger>            mLogger;
    };

} // namespace winsetup::application
