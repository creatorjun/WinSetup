// src/application/viewmodels/MainViewModel.cpp
#include <application/viewmodels/MainViewModel.h>

namespace winsetup::application {

    MainViewModel::MainViewModel(
        std::shared_ptr<LoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mLoadConfigUseCase(std::move(loadConfigUseCase))
        , mLogger(std::move(logger))
        , mConfig(nullptr)
        , mStatusText(L"Ready")
        , mWindowTitle(L"WinSetup - PC Reinstallation Tool")
        , mIsInitializing(false)
        , mIsProcessing(false)
        , mIsCompleted(false)
    {
    }

    std::wstring MainViewModel::GetStatusText() const {
        return mStatusText;
    }

    void MainViewModel::SetStatusText(const std::wstring& text) {
        if (mStatusText != text) {
            mStatusText = text;
            NotifyPropertyChanged(L"StatusText");
        }
    }

    std::wstring MainViewModel::GetWindowTitle() const {
        return mWindowTitle;
    }

    void MainViewModel::SetWindowTitle(const std::wstring& title) {
        if (mWindowTitle != title) {
            mWindowTitle = title;
            NotifyPropertyChanged(L"WindowTitle");
        }
    }

    domain::Expected<void> MainViewModel::Initialize() {
        mIsInitializing = true;
        SetStatusText(L"Initializing...");

        if (mLogger)
            mLogger->Info(L"MainViewModel initialization started");

        auto configResult = LoadConfiguration();
        if (!configResult.HasValue()) {
            mIsInitializing = false;
            SetStatusText(L"Failed to load configuration");
            return configResult.GetError();
        }

        mIsInitializing = false;
        SetStatusText(L"Ready");

        if (mLogger)
            mLogger->Info(L"MainViewModel initialization completed");

        return domain::Expected<void>{};
    }

    domain::Expected<void> MainViewModel::LoadConfiguration() {
        auto result = mLoadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue())
            return result.GetError();
        mConfig = result.Value();
        return domain::Expected<void>{};
    }

    void MainViewModel::AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) {
        mPropertyChangedHandlers.push_back(std::move(callback));
    }

    void MainViewModel::RemoveAllPropertyChangedHandlers() {
        mPropertyChangedHandlers.clear();
    }

    void MainViewModel::NotifyPropertyChanged(const std::wstring& propertyName) {
        for (const auto& handler : mPropertyChangedHandlers)
            handler(propertyName);
    }

}
