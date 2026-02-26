#include "main/ServiceRegistration.h"
#include "application/core/DIContainer.h"
#include "adapters/platform/win32/logging/Win32Logger.h"
#include "adapters/platform/win32/system/Win32SystemInfoService.h"
#include "adapters/platform/win32/storage/Win32DiskService.h"
#include "adapters/platform/win32/storage/Win32VolumeService.h"
#include "adapters/platform/win32/storage/Win32FileCopyService.h"
#include "adapters/persistence/config/IniConfigRepository.h"
#include <application/repositories/AnalysisRepository.h>
#include "adapters/persistence/filesystem/Win32PathChecker.h"
#include "adapters/ui/win32/Win32MainWindow.h"
#include "application/usecases/system/AnalyzeSystemUseCase.h"
#include "application/usecases/system/LoadConfigurationUseCase.h"
#include "application/usecases/disk/EnumerateDisksUseCase.h"
#include "application/usecases/disk/EnumerateVolumesUseCase.h"
#include "application/usecases/disk/AnalyzeVolumesUseCase.h"
#include "application/viewmodels/MainViewModel.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "abstractions/repositories/IConfigRepository.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/services/platform/ISystemInfoService.h"
#include "abstractions/services/storage/IDiskService.h"
#include "abstractions/services/storage/IVolumeService.h"
#include "abstractions/services/storage/IFileCopyService.h"
#include "abstractions/services/storage/IPathChecker.h"
#include "abstractions/usecases/IAnalyzeSystemUseCase.h"
#include "abstractions/usecases/IAnalyzeVolumesUseCase.h"
#include "abstractions/usecases/ILoadConfigurationUseCase.h"
#include "abstractions/usecases/IEnumerateDisksUseCase.h"
#include "abstractions/usecases/IEnumerateVolumesUseCase.h"
#include "abstractions/ui/IMainViewModel.h"
#include "abstractions/ui/IWindow.h"
#include <stdexcept>
#include <string>

namespace winsetup {

    namespace {
        template<typename TInterface>
        std::shared_ptr<TInterface> ResolveOrThrow(
            application::DIContainer& container,
            const std::string& name)
        {
            auto result = container.Resolve<TInterface>();
            if (!result.HasValue())
                throw std::runtime_error("Failed to resolve: " + name);
            return result.Value();
        }
    }

    void ServiceRegistration::RegisterAllServices(
        application::DIContainer& container,
        HINSTANCE hInstance)
    {
        RegisterInfrastructureServices(container);
        RegisterDomainServices(container);
        RegisterRepositoryServices(container);
        RegisterPlatformServices(container);
        RegisterStorageServices(container);
        RegisterUseCaseServices(container);
        RegisterApplicationServices(container);
        RegisterUIServices(container, hInstance);
    }

    void ServiceRegistration::RegisterInfrastructureServices(
        application::DIContainer& container)
    {
        auto logger = std::make_shared<adapters::platform::Win32Logger>(L"log\\log.txt");
        container.RegisterInstance<abstractions::ILogger>(
            std::static_pointer_cast<abstractions::ILogger>(logger));
    }

    void ServiceRegistration::RegisterDomainServices(
        application::DIContainer& container)
    {
    }

    void ServiceRegistration::RegisterRepositoryServices(
        application::DIContainer& container)
    {
        container.RegisterInstance<abstractions::IConfigRepository>(
            std::static_pointer_cast<abstractions::IConfigRepository>(
                std::make_shared<adapters::persistence::IniConfigRepository>()));

        container.RegisterInstance<abstractions::IAnalysisRepository>(
            std::static_pointer_cast<abstractions::IAnalysisRepository>(
                std::make_shared<adapters::persistence::AnalysisRepository>()));
    }

    void ServiceRegistration::RegisterPlatformServices(
        application::DIContainer& container)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        container.RegisterInstance<abstractions::ISystemInfoService>(
            std::static_pointer_cast<abstractions::ISystemInfoService>(
                std::make_shared<adapters::platform::Win32SystemInfoService>(logger)));
    }

    void ServiceRegistration::RegisterStorageServices(
        application::DIContainer& container)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");

        container.RegisterInstance<abstractions::IDiskService>(
            std::static_pointer_cast<abstractions::IDiskService>(
                std::make_shared<adapters::platform::Win32DiskService>(logger)));

        container.RegisterInstance<abstractions::IVolumeService>(
            std::static_pointer_cast<abstractions::IVolumeService>(
                std::make_shared<adapters::platform::Win32VolumeService>(logger)));

        container.RegisterInstance<abstractions::IFileCopyService>(
            std::static_pointer_cast<abstractions::IFileCopyService>(
                std::make_shared<adapters::platform::Win32FileCopyService>(logger)));

        container.RegisterInstance<abstractions::IPathChecker>(
            std::static_pointer_cast<abstractions::IPathChecker>(
                std::make_shared<adapters::persistence::Win32PathChecker>()));
    }

    void ServiceRegistration::RegisterUseCaseServices(
        application::DIContainer& container)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto configRepo = ResolveOrThrow<abstractions::IConfigRepository>(container, "IConfigRepository");
        auto analysis = ResolveOrThrow<abstractions::IAnalysisRepository>(container, "IAnalysisRepository");
        auto sysInfo = ResolveOrThrow<abstractions::ISystemInfoService>(container, "ISystemInfoService");
        auto diskService = ResolveOrThrow<abstractions::IDiskService>(container, "IDiskService");
        auto volService = ResolveOrThrow<abstractions::IVolumeService>(container, "IVolumeService");
        auto pathChecker = ResolveOrThrow<abstractions::IPathChecker>(container, "IPathChecker");

        auto loadConfig = std::make_shared<application::LoadConfigurationUseCase>(configRepo, logger);
        container.RegisterInstance<abstractions::ILoadConfigurationUseCase>(
            std::static_pointer_cast<abstractions::ILoadConfigurationUseCase>(loadConfig));

        auto enumerateDisks = std::make_shared<application::EnumerateDisksUseCase>(diskService, logger);
        container.RegisterInstance<abstractions::IEnumerateDisksUseCase>(
            std::static_pointer_cast<abstractions::IEnumerateDisksUseCase>(enumerateDisks));

        auto enumerateVolumes = std::make_shared<application::EnumerateVolumesUseCase>(volService, logger);
        container.RegisterInstance<abstractions::IEnumerateVolumesUseCase>(
            std::static_pointer_cast<abstractions::IEnumerateVolumesUseCase>(enumerateVolumes));

        auto analyzeVolumes = std::make_shared<application::AnalyzeVolumesUseCase>(
            analysis, configRepo, pathChecker, logger);
        container.RegisterInstance<abstractions::IAnalyzeVolumesUseCase>(
            std::static_pointer_cast<abstractions::IAnalyzeVolumesUseCase>(analyzeVolumes));

        container.RegisterInstance<abstractions::IAnalyzeSystemUseCase>(
            std::static_pointer_cast<abstractions::IAnalyzeSystemUseCase>(
                std::make_shared<application::AnalyzeSystemUseCase>(
                    sysInfo,
                    loadConfig,
                    enumerateDisks,
                    enumerateVolumes,
                    analyzeVolumes,
                    analysis,
                    logger)));
    }


    void ServiceRegistration::RegisterApplicationServices(
        application::DIContainer& container)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto loadConfig = ResolveOrThrow<abstractions::ILoadConfigurationUseCase>(container, "ILoadConfigurationUseCase");
        auto analyze = ResolveOrThrow<abstractions::IAnalyzeSystemUseCase>(container, "IAnalyzeSystemUseCase");
        auto configRepo = ResolveOrThrow<abstractions::IConfigRepository>(container, "IConfigRepository");
        auto analysis = ResolveOrThrow<abstractions::IAnalysisRepository>(container, "IAnalysisRepository");

        container.RegisterInstance<abstractions::IMainViewModel>(
            std::static_pointer_cast<abstractions::IMainViewModel>(
                std::make_shared<application::MainViewModel>(
                    loadConfig, analyze, configRepo, analysis, logger)));
    }

    void ServiceRegistration::RegisterUIServices(
        application::DIContainer& container,
        HINSTANCE hInstance)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto viewModel = ResolveOrThrow<abstractions::IMainViewModel>(container, "IMainViewModel");

        auto window = std::make_shared<adapters::ui::Win32MainWindow>(logger, viewModel);
        window->Create(hInstance, SW_SHOWDEFAULT);
        container.RegisterInstance<abstractions::IWindow>(
            std::static_pointer_cast<abstractions::IWindow>(window));
    }

} // namespace winsetup
