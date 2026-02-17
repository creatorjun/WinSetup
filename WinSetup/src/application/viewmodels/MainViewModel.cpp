// src/application/viewmodels/MainViewModel.cpp

#include <application/viewmodels/MainViewModel.h>

namespace winsetup::application {

    MainViewModel::MainViewModel(
        std::shared_ptr<LoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::ILogger>    logger)
        : m_loadConfigUseCase(std::move(loadConfigUseCase))
        , m_logger(std::move(logger))
        , m_config(nullptr)
        , m_statusText(L"Ready")
        , m_windowTitle(L"WinSetup - PC Reinstallation Tool")
        , m_typeDescription(L"")
        , m_isInitializing(false)
        , m_isProcessing(false)
        , m_isCompleted(false)
        , m_dataPreservation(false)
        , m_bitlockerEnabled(false)
    {
    }

    std::wstring MainViewModel::GetStatusText() const { return m_statusText; }

    void MainViewModel::SetStatusText(const std::wstring& text) {
        if (m_statusText != text) {
            m_statusText = text;
            NotifyPropertyChanged(L"StatusText");
        }
    }

    std::wstring MainViewModel::GetWindowTitle() const { return m_windowTitle; }

    void MainViewModel::SetWindowTitle(const std::wstring& title) {
        if (m_windowTitle != title) {
            m_windowTitle = title;
            NotifyPropertyChanged(L"WindowTitle");
        }
    }

    std::vector<domain::InstallationType> MainViewModel::GetInstallationTypes() const {
        if (!m_config) return {};
        return m_config->GetInstallationTypes();
    }

    std::wstring MainViewModel::GetTypeDescription() const {
        return m_typeDescription;
    }

    void MainViewModel::SetTypeDescription(const std::wstring& key) {
        if (!m_config) return;

        const auto& types = m_config->GetInstallationTypes();
        for (const auto& t : types) {
            if (t.name == key) {
                if (m_typeDescription != t.description) {
                    m_typeDescription = t.description;
                    NotifyPropertyChanged(L"TypeDescription");
                }
                return;
            }
        }
    }

    bool MainViewModel::GetDataPreservation() const {
        return m_dataPreservation;
    }

    void MainViewModel::SetDataPreservation(bool enabled) {
        if (m_dataPreservation != enabled) {
            m_dataPreservation = enabled;
            NotifyPropertyChanged(L"DataPreservation");
        }
    }

    bool MainViewModel::GetBitlockerEnabled() const {
        return m_bitlockerEnabled;
    }

    void MainViewModel::SetBitlockerEnabled(bool enabled) {
        if (m_bitlockerEnabled != enabled) {
            m_bitlockerEnabled = enabled;
            NotifyPropertyChanged(L"BitlockerEnabled");
        }
    }

    domain::Expected<void> MainViewModel::Initialize() {
        m_isInitializing = true;
        SetStatusText(L"Initializing...");

        if (m_logger) m_logger->Info(L"MainViewModel initialization started");

        auto configResult = LoadConfiguration();
        if (!configResult.HasValue()) {
            m_isInitializing = false;
            SetStatusText(L"Failed to load configuration");
            return configResult;
        }

        m_isInitializing = false;
        SetStatusText(L"Ready");

        NotifyPropertyChanged(L"InstallationTypes");

        if (m_logger) m_logger->Info(L"MainViewModel initialization completed");
        return domain::Expected<void>();
    }

    domain::Expected<void> MainViewModel::LoadConfiguration() {
        auto result = m_loadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue()) {
            if (m_logger)
                m_logger->Error(L"Failed to load configuration: " + result.GetError().GetMessage());
            return result.GetError();
        }
        m_config = result.Value();
        return domain::Expected<void>();
    }

    void MainViewModel::AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) {
        m_propertyChangedHandlers.push_back(std::move(callback));
    }

    void MainViewModel::RemoveAllPropertyChangedHandlers() {
        m_propertyChangedHandlers.clear();
    }

    void MainViewModel::NotifyPropertyChanged(const std::wstring& propertyName) {
        for (const auto& handler : m_propertyChangedHandlers)
            handler(propertyName);
    }

}
