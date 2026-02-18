// src/main/ServiceRegistration.cpp
#include "ServiceRegistration.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "abstractions/repositories/IConfigRepository.h"
#include "abstractions/services/platform/ISystemInfoService.h"
#include "abstractions/services/storage/IFileCopyService.h"
#include "abstractions/ui/IMainViewModel.h"
#include "abstractions/ui/IWindow.h"
#include "adapters/persistence/config/IniConfigRepository.h"
#include "adapters/platform/win32/logging/Win32Logger.h"
#include "adapters/platform/win32/storage/Win32FileCopyService.h"
#include "adapters/platform/win32/system/Win32SystemInfoService.h"
#include "adapters/ui/win32/Win32MainWindow.h"
#include "application/usecases/system/AnalyzeSystemUseCase.h"
#include "application/usecases/system/LoadConfigurationUseCase.h"
#include "application/viewmodels/MainViewModel.h"

namespace winsetup {

    void ServiceRegistration::RegisterAllServices(
        application::DIContainer& container,
        HINSTANCE hInstance
    ) {
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
        application::DIContainer& container
    ) {
        auto logger = std::make_shared<adapters::platform::Win32Logger>(L"log/log.txt");
        container.RegisterInstance<abstractions::ILogger>(
            std::static_pointer_cast<abstractions::ILogger>(logger));
    }

    void ServiceRegistration::RegisterDomainServices(
        application::DIContainer& container
    ) {
    }

    void ServiceRegistration::RegisterRepositoryServices(
        application::DIContainer& container
    ) {
        container.RegisterInstance<abstractions::IConfigRepository>(
            std::static_pointer_cast<abstractions::IConfigRepository>(
                std::make_shared<adapters::persistence::IniConfigRepository>()));
    }

    void ServiceRegistration::RegisterPlatformServices(
        application::DIContainer& container
    ) {
        auto loggerResult = container.Resolve<abstractions::ILogger>();
        auto logger = loggerResult.HasValue() ? loggerResult.Value() : nullptr;

        container.RegisterInstance<abstractions::ISystemInfoService>(
            std::static_pointer_cast<abstractions::ISystemInfoService>(
                std::make_shared<adapters::platform::Win32SystemInfoService>(logger)));
    }

    void ServiceRegistration::RegisterStorageServices(
        application::DIContainer& container
    ) {
        auto loggerResult = container.Resolve<abstractions::ILogger>();
        auto logger = loggerResult.HasValue() ? loggerResult.Value() : nullptr;

        container.RegisterInstance<abstractions::IFileCopyService>(
            std::static_pointer_cast<abstractions::IFileCopyService>(
                std::make_shared<adapters::platform::Win32FileCopyService>(logger)));
    }

    void ServiceRegistration::RegisterUseCaseServices(
        application::DIContainer& container
    ) {
        auto loggerResult = container.Resolve<abstractions::ILogger>();
        auto repoResult = container.Resolve<abstractions::IConfigRepository>();
        auto sysInfoResult = container.Resolve<abstractions::ISystemInfoService>();

        auto logger = loggerResult.HasValue() ? loggerResult.Value() : nullptr;
        auto repo = repoResult.HasValue() ? repoResult.Value() : nullptr;
        auto sysInfo = sysInfoResult.HasValue() ? sysInfoResult.Value() : nullptr;

        container.RegisterInstance<application::LoadConfigurationUseCase>(
            std::make_shared<application::LoadConfigurationUseCase>(repo, logger));

        container.RegisterInstance<application::AnalyzeSystemUseCase>(
            std::make_shared<application::AnalyzeSystemUseCase>(sysInfo, logger));
    }

    void ServiceRegistration::RegisterApplicationServices(
        application::DIContainer& container
    ) {
        auto loggerResult = container.Resolve<abstractions::ILogger>();
        auto loadConfigResult = container.Resolve<application::LoadConfigurationUseCase>();
        auto analyzeResult = container.Resolve<application::AnalyzeSystemUseCase>();

        auto logger = loggerResult.HasValue() ? loggerResult.Value() : nullptr;
        auto loadConfigUC = loadConfigResult.HasValue() ? loadConfigResult.Value() : nullptr;
        auto analyzeUC = analyzeResult.HasValue() ? analyzeResult.Value() : nullptr;

        container.RegisterInstance<abstractions::IMainViewModel>(
            std::static_pointer_cast<abstractions::IMainViewModel>(
                std::make_shared<application::MainViewModel>(
                    loadConfigUC, analyzeUC, logger)));
    }

    void ServiceRegistration::RegisterUIServices(
        application::DIContainer& container,
        HINSTANCE hInstance
    ) {
        auto loggerResult = container.Resolve<abstractions::ILogger>();
        auto viewModelResult = container.Resolve<abstractions::IMainViewModel>();

        if (!loggerResult.HasValue() || !viewModelResult.HasValue()) return;

        auto logger = loggerResult.Value();
        auto viewModel = viewModelResult.Value();

        auto window = std::make_shared<adapters::ui::Win32MainWindow>(logger, viewModel);
        window->Create(hInstance, SW_SHOWDEFAULT);

        container.RegisterInstance<abstractions::IWindow>(
            std::static_pointer_cast<abstractions::IWindow>(window));
    }

}
