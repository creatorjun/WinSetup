#pragma once
#include "abstractions/usecases/ISetupSystemUseCase.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include <memory>

namespace winsetup::application {

    class SetupSystemUseCase final : public abstractions::ISetupSystemUseCase {
    public:
        explicit SetupSystemUseCase(
            std::shared_ptr<abstractions::ILogger> logger);
        ~SetupSystemUseCase() override = default;
        SetupSystemUseCase(const SetupSystemUseCase&) = delete;
        SetupSystemUseCase& operator=(const SetupSystemUseCase&) = delete;

        [[nodiscard]] domain::Expected<void> Execute(
            std::shared_ptr<const domain::SetupConfig> config) override;

    private:
        std::shared_ptr<abstractions::ILogger> mLogger;
    };

} // namespace winsetup::application
