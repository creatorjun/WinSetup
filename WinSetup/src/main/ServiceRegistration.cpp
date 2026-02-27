#include "main/ServiceRegistration.h"
#include "application/core/DIContainer.h"
#include "adapters/platform/win32/logging/Win32Logger.h"
#include "adapters/platform/win32/system/Win32SystemInfoService.h"
#include "adapters/platform/win32/storage/Win32DiskService.h"
#include "adapters/platform/win32/storage/Win32VolumeService.h"
#include "adapters/platform/win32/storage/Win32FileCopyService.h"
#include "adapters/persistence/config/IniConfigRepository.h"
#include "application/repositories/AnalysisRepository.h"
#include "adapters/persistence/filesystem/Win32PathChecker.h"
#include "adapters/ui/win32/Win32MainWindow.h"
#include "application/usecases/system/AnalyzeSystemUseCase.h"
#include "application/usecases/system/LoadConfigurationUseCase.h"
#include "application/usecases/disk/EnumerateDisksStep.h"
#include "application/usecases/disk/EnumerateVolumesStep.h"
#include "application/usecases/disk/AnalyzeVolumesStep.h"
#include "application/usecases/disk/AnalyzeDisksStep.h"
#include "application/usecases/install/SetupSystemUseCase.h"
#include "application/usecases/install/BackupDataStep.h"
#include "application/usecases/install/FormatPartitionStep.h"
#include "application/usecases/install/ApplyImageStep.h"
#include "application/usecases/install/InstallDriversStep.h"
#include "application/usecases/install/RestoreDataStep.h"
#include "application/usecases/install/ProvisioningStep.h"
#include "application/usecases/install/RebootStep.h"
#include "application/viewmodels/MainViewModel.h"
#include "application/services/Dispatcher.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "abstractions/repositories/IConfigRepository.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/services/platform/ISystemInfoService.h"
#include "abstractions/services/storage/IDiskService.h"
#include "abstractions/services/storage/IVolumeService.h"
#include "abstractions/services/storage/IFileCopyService.h"
#include "abstractions/services/storage/IPathChecker.h"
#include "abstractions/usecases/IAnalyzeSystemUseCase.h"
#include "abstractions/usecases/ILoadConfigurationUseCase.h"
#include "abstractions/usecases/ISetupSystemUseCase.h"
#include "abstractions/usecases/steps/IEnumerateDisksStep.h"
#include "abstractions/usecases/steps/IEnumerateVolumesStep.h"
#include "abstractions/usecases/steps/IAnalyzeVolumesStep.h"
#include "abstractions/usecases/steps/IAnalyzeDisksStep.h"
#include "abstractions/usecases/steps/IBackupDataStep.h"
#include "abstractions/usecases/steps/IFormatPartitionStep.h"
#include "abstractions/usecases/steps/IApplyImageStep.h"
#include "abstractions/usecases/steps/IInstallDriversStep.h"
#include "abstractions/usecases/steps/IRestoreDataStep.h"
#include "abstractions/usecases/steps/IProvisioningStep.h"
#include "abstractions/usecases/steps/IRebootStep.h"
#include "abstractions/ui/IUIDispatcher.h"
#include "abstractions/ui/IMainViewModel.h"
#include "abstractions/ui/IWindow.h"
#include <stdexcept>
#include <string>

namespace winsetup {

    namespace {
        template<typename TInterface>
        std::shared_ptr<TInterface> ResolveOrThrow(
            application::DIContainer& container, const std::string& name)
        {
            auto result = container.Resolve<TInterface>();
            if (!result.HasValue())
                throw std::runtime_error("Failed to resolve " + name);
            return result.Value();
        }
    }

    void ServiceRegistration::RegisterAllServices(
        application::DIContainer& container, HINSTANCE hInstance)
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

    void ServiceRegistration::RegisterInfrastructureServices(application::DIContainer& container)
    {
        CreateDirectoryW(L"log", nullptr);
        auto logger = std::make_shared<adapters::platform::Win32Logger>(L"log/log.txt");
        container.RegisterInstance<abstractions::ILogger>(
            std::static_pointer_cast<abstractions::ILogger>(logger));

        auto dispatcher = std::make_shared<application::Dispatcher>();
        container.RegisterInstance<abstractions::IUIDispatcher>(
            std::static_pointer_cast<abstractions::IUIDispatcher>(dispatcher));
        container.RegisterInstance<application::Dispatcher>(dispatcher);
    }

    void ServiceRegistration::RegisterDomainServices(application::DIContainer& container) {}

    void ServiceRegistration::RegisterRepositoryServices(application::DIContainer& container)
    {
        container.RegisterInstance<abstractions::IConfigRepository>(
            std::static_pointer_cast<abstractions::IConfigRepository>(
                std::make_shared<adapters::persistence::IniConfigRepository>()));
        container.RegisterInstance<abstractions::IAnalysisRepository>(
            std::static_pointer_cast<abstractions::IAnalysisRepository>(
                std::make_shared<adapters::persistence::AnalysisRepository>()));
    }

    void ServiceRegistration::RegisterPlatformServices(application::DIContainer& container)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        container.RegisterInstance<abstractions::ISystemInfoService>(
            std::static_pointer_cast<abstractions::ISystemInfoService>(
                std::make_shared<adapters::platform::Win32SystemInfoService>(logger)));
    }

    void ServiceRegistration::RegisterStorageServices(application::DIContainer& container)
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

    void ServiceRegistration::RegisterUseCaseServices(application::DIContainer& container)
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

        auto enumerateDisks = std::make_shared<application::EnumerateDisksStep>(diskService, logger);
        container.RegisterInstance<abstractions::IEnumerateDisksStep>(
            std::static_pointer_cast<abstractions::IEnumerateDisksStep>(enumerateDisks));

        auto enumerateVolumes = std::make_shared<application::EnumerateVolumesStep>(volService, logger);
        container.RegisterInstance<abstractions::IEnumerateVolumesStep>(
            std::static_pointer_cast<abstractions::IEnumerateVolumesStep>(enumerateVolumes));

        auto analyzeVolumes = std::make_shared<application::AnalyzeVolumesStep>(
            analysis, configRepo, pathChecker, logger);
        container.RegisterInstance<abstractions::IAnalyzeVolumesStep>(
            std::static_pointer_cast<abstractions::IAnalyzeVolumesStep>(analyzeVolumes));

        auto analyzeDisks = std::make_shared<application::AnalyzeDisksStep>(analysis, logger);
        container.RegisterInstance<abstractions::IAnalyzeDisksStep>(
            std::static_pointer_cast<abstractions::IAnalyzeDisksStep>(analyzeDisks));

        container.RegisterInstance<abstractions::IAnalyzeSystemUseCase>(
            std::static_pointer_cast<abstractions::IAnalyzeSystemUseCase>(
                std::make_shared<application::AnalyzeSystemUseCase>(
                    sysInfo,
                    enumerateDisks, enumerateVolumes,
                    analyzeVolumes, analyzeDisks,
                    analysis, configRepo, logger)));

        auto backupData = std::make_shared<application::BackupDataStep>(logger);
        auto formatPartition = std::make_shared<application::FormatPartitionStep>(logger);
        auto applyImage = std::make_shared<application::ApplyImageStep>(logger);
        auto installDrivers = std::make_shared<application::InstallDriversStep>(logger);
        auto restoreData = std::make_shared<application::RestoreDataStep>(logger);
        auto provisioning = std::make_shared<application::ProvisioningStep>(logger);
        auto reboot = std::make_shared<application::RebootStep>(logger);

        container.RegisterInstance<abstractions::IBackupDataStep>(
            std::static_pointer_cast<abstractions::IBackupDataStep>(backupData));
        container.RegisterInstance<abstractions::IFormatPartitionStep>(
            std::static_pointer_cast<abstractions::IFormatPartitionStep>(formatPartition));
        container.RegisterInstance<abstractions::IApplyImageStep>(
            std::static_pointer_cast<abstractions::IApplyImageStep>(applyImage));
        container.RegisterInstance<abstractions::IInstallDriversStep>(
            std::static_pointer_cast<abstractions::IInstallDriversStep>(installDrivers));
        container.RegisterInstance<abstractions::IRestoreDataStep>(
            std::static_pointer_cast<abstractions::IRestoreDataStep>(restoreData));
        container.RegisterInstance<abstractions::IProvisioningStep>(
            std::static_pointer_cast<abstractions::IProvisioningStep>(provisioning));
        container.RegisterInstance<abstractions::IRebootStep>(
            std::static_pointer_cast<abstractions::IRebootStep>(reboot));

        container.RegisterInstance<abstractions::ISetupSystemUseCase>(
            std::static_pointer_cast<abstractions::ISetupSystemUseCase>(
                std::make_shared<application::SetupSystemUseCase>(
                    backupData, formatPartition, applyImage, installDrivers,
                    restoreData, provisioning, reboot, logger)));
    }

    void ServiceRegistration::RegisterApplicationServices(application::DIContainer& container)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto loadConfig = ResolveOrThrow<abstractions::ILoadConfigurationUseCase>(container, "ILoadConfigurationUseCase");
        auto analyze = ResolveOrThrow<abstractions::IAnalyzeSystemUseCase>(container, "IAnalyzeSystemUseCase");
        auto setupSystem = ResolveOrThrow<abstractions::ISetupSystemUseCase>(container, "ISetupSystemUseCase");
        auto configRepo = ResolveOrThrow<abstractions::IConfigRepository>(container, "IConfigRepository");
        auto analysis = ResolveOrThrow<abstractions::IAnalysisRepository>(container, "IAnalysisRepository");
        auto dispatcher = ResolveOrThrow<abstractions::IUIDispatcher>(container, "IUIDispatcher");

        container.RegisterInstance<abstractions::IMainViewModel>(
            std::static_pointer_cast<abstractions::IMainViewModel>(
                std::make_shared<application::MainViewModel>(
                    loadConfig, analyze, setupSystem,
                    configRepo, analysis, dispatcher, logger)));
    }

    void ServiceRegistration::RegisterUIServices(
        application::DIContainer& container, HINSTANCE hInstance)
    {
        auto logger = ResolveOrThrow<abstractions::ILogger>(container, "ILogger");
        auto viewModel = ResolveOrThrow<abstractions::IMainViewModel>(container, "IMainViewModel");
        auto dispatcher = ResolveOrThrow<application::Dispatcher>(container, "Dispatcher");

        auto window = std::make_shared<adapters::ui::Win32MainWindow>(
            logger, viewModel, dispatcher);
        window->Create(hInstance, SW_SHOWDEFAULT);
        container.RegisterInstance<abstractions::IWindow>(
            std::static_pointer_cast<abstractions::IWindow>(window));
    }

} // namespace winsetup
