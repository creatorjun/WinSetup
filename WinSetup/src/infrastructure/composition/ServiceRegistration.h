#pragma once

#include <memory>
#include "DependencyContainer.h"
#include "../../abstractions/platform/ITextEncoder.h"
#include "../../abstractions/platform/ISystemInfoService.h"
#include "../../abstractions/storage/IDiskService.h"
#include "../../abstractions/storage/IVolumeService.h"
#include "../../abstractions/storage/IPartitionService.h"
#include "../../abstractions/storage/IStorageScanner.h"
#include "../../abstractions/logging/ILogger.h"

#include "../../adapters/platform/windows/encoding/Win32TextEncoder.h"
#include "../../adapters/platform/windows/system/Win32SystemInfoService.h"
#include "../../adapters/platform/windows/storage/Win32DiskService.h"
#include "../../adapters/platform/windows/storage/Win32VolumeService.h"
#include "../../adapters/platform/windows/storage/Win32PartitionService.h"
#include "../../adapters/platform/windows/storage/Win32StorageScanner.h"

#include "../logging/WindowsLogger.h"

namespace winsetup::infrastructure {

    class ServiceRegistration {
    public:
        static void RegisterAll(DependencyContainer& container) {
            RegisterInfrastructure(container);
            RegisterAdapters(container);
        }

    private:
        static void RegisterInfrastructure(DependencyContainer& container) {
            auto logger = std::make_shared<WindowsLogger>();
            container.RegisterInstance<abstractions::ILogger>(logger);
        }

        static void RegisterAdapters(DependencyContainer& container) {
            container.RegisterFactory<abstractions::ITextEncoder>(
                []() {
                    return std::make_shared<adapters::Win32TextEncoder>();
                },
                ServiceLifetime::Singleton
            );

            container.RegisterFactory<abstractions::ISystemInfoService>(
                [&container]() {
                    auto encoder = container.Resolve<abstractions::ITextEncoder>();
                    return std::make_shared<adapters::Win32SystemInfoService>(encoder);
                },
                ServiceLifetime::Singleton
            );

            container.RegisterFactory<abstractions::IDiskService>(
                [&container]() {
                    auto encoder = container.Resolve<abstractions::ITextEncoder>();
                    auto logger = container.Resolve<abstractions::ILogger>();
                    return std::make_shared<adapters::Win32DiskService>(encoder, logger);
                },
                ServiceLifetime::Singleton
            );

            container.RegisterFactory<abstractions::IVolumeService>(
                [&container]() {
                    auto encoder = container.Resolve<abstractions::ITextEncoder>();
                    return std::make_shared<adapters::Win32VolumeService>(encoder);
                },
                ServiceLifetime::Singleton
            );

            container.RegisterFactory<abstractions::IPartitionService>(
                [&container]() {
                    auto encoder = container.Resolve<abstractions::ITextEncoder>();
                    return std::make_shared<adapters::Win32PartitionService>(encoder);
                },
                ServiceLifetime::Singleton
            );

            container.RegisterFactory<abstractions::IStorageScanner>(
                [&container]() {
                    auto volumeService = container.Resolve<abstractions::IVolumeService>();
                    auto encoder = container.Resolve<abstractions::ITextEncoder>();
                    return std::make_shared<adapters::Win32StorageScanner>(
                        volumeService,
                        encoder
                    );
                },
                ServiceLifetime::Singleton
            );
        }
    };

}
