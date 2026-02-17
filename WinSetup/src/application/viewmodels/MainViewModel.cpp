// src/application/viewmodels/MainViewModel.cpp
#include <application/viewmodels/MainViewModel.h>

namespace winsetup::application {

    MainViewModel::MainViewModel(
        std::shared_ptr<LoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::ILogger> logger)
        : m_loadConfigUseCase(std::move(loadConfigUseCase))
        , m_logger(std::move(logger))
        , m_config(nullptr)
        , m_statusText(L"Ready")
        , m_windowTitle(L"WinSetup - PC Reinstallation Tool")
        , m_typeDescription(L"타입을 선택해주세요.")
        , m_isInitializing(false)
        , m_isProcessing(false)
        , m_isCompleted(false)
    {
    }

    std::wstring MainViewModel::GetStatusText() const {
        return m_statusText;
    }

    void MainViewModel::SetStatusText(const std::wstring& text) {
        if (m_statusText != text) {
            m_statusText = text;
            NotifyPropertyChanged(L"StatusText");
        }
    }

    std::wstring MainViewModel::GetWindowTitle() const {
        return m_windowTitle;
    }

    void MainViewModel::SetWindowTitle(const std::wstring& title) {
        if (m_windowTitle != title) {
            m_windowTitle = title;
            NotifyPropertyChanged(L"WindowTitle");
        }
    }

    std::wstring MainViewModel::GetTypeDescription() const {
        return m_typeDescription;
    }

    void MainViewModel::SetTypeDescription(const std::wstring& description) {
        if (m_typeDescription != description) {
            m_typeDescription = description;
            NotifyPropertyChanged(L"TypeDescription");
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
            return configResult.GetError();
        }

        m_isInitializing = false;
        SetStatusText(L"Ready");

        if (m_logger) m_logger->Info(L"MainViewModel initialization completed");
        return domain::Expected<void>();
    }

    domain::Expected<void> MainViewModel::LoadConfiguration() {
        auto result = m_loadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue()) {
            if (m_logger) m_logger->Error(L"Failed to load configuration: " + result.GetError().GetMessage());
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
