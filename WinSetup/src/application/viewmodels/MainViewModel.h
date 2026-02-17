// src/application/viewmodels/MainViewModel.h
#pragma once

#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <application/usecases/system/LoadConfigurationUseCase.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <vector>
#include <string>

namespace winsetup::application {

    class MainViewModel : public abstractions::IMainViewModel {
    public:
        explicit MainViewModel(
            std::shared_ptr<LoadConfigurationUseCase> loadConfigUseCase,
            std::shared_ptr<abstractions::ILogger>    logger);

        ~MainViewModel() override = default;

        [[nodiscard]] std::wstring GetStatusText()  const override;
        void SetStatusText(const std::wstring& text)      override;

        [[nodiscard]] std::wstring GetWindowTitle() const override;
        void SetWindowTitle(const std::wstring& title)    override;

        [[nodiscard]] std::vector<domain::InstallationType> GetInstallationTypes() const override;

        [[nodiscard]] std::wstring GetTypeDescription() const override;
        void SetTypeDescription(const std::wstring& key)      override;

        [[nodiscard]] bool GetDataPreservation() const override;
        void SetDataPreservation(bool enabled)         override;

        [[nodiscard]] bool GetBitlockerEnabled() const override;
        void SetBitlockerEnabled(bool enabled)        override;

        [[nodiscard]] bool IsInitializing() const override { return m_isInitializing; }
        [[nodiscard]] bool IsProcessing()   const override { return m_isProcessing; }
        [[nodiscard]] bool IsCompleted()    const override { return m_isCompleted; }

        void SetProcessing(bool processing) override;

        domain::Expected<void> Initialize() override;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers()                                        override;

        [[nodiscard]] std::shared_ptr<domain::SetupConfig> GetConfig() const { return m_config; }

    protected:
        void NotifyPropertyChanged(const std::wstring& propertyName) override;

    private:
        domain::Expected<void> LoadConfiguration();

        std::shared_ptr<LoadConfigurationUseCase> m_loadConfigUseCase;
        std::shared_ptr<abstractions::ILogger>    m_logger;
        std::shared_ptr<domain::SetupConfig>      m_config;

        std::wstring m_statusText;
        std::wstring m_windowTitle;
        std::wstring m_typeDescription;

        bool m_isInitializing = false;
        bool m_isProcessing = false;
        bool m_isCompleted = false;
        bool m_dataPreservation = false;
        bool m_bitlockerEnabled = false;

        std::vector<abstractions::PropertyChangedCallback> m_propertyChangedHandlers;
    };

}
