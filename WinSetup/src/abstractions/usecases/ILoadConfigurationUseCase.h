// src/abstractions/usecases/ILoadConfigurationUseCase.h
#pragma once
#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <string>

namespace winsetup::abstractions {

    class ILoadConfigurationUseCase {
    public:
        virtual ~ILoadConfigurationUseCase() = default;
        [[nodiscard]] virtual domain::Expected<std::shared_ptr<domain::SetupConfig>>
            Execute(const std::wstring& configPath = L"config.ini") = 0;
    };

} // namespace winsetup::abstractions
