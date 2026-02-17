// src/abstractions/repositories/IConfigRepository.h
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <string>

namespace winsetup::abstractions {

    class IConfigRepository {
    public:
        virtual ~IConfigRepository() = default;

        [[nodiscard]] virtual domain::Expected<std::shared_ptr<domain::SetupConfig>>
            LoadConfig(const std::wstring& filePath) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            SaveConfig(const std::wstring& filePath, const domain::SetupConfig& config) = 0;
    };

}
