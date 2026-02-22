// src/application/viewmodels/MainViewModel.cpp
#include "application/viewmodels/MainViewModel.h"

namespace winsetup::application {

    MainViewModel::MainViewModel(
        std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     analyzeSystemUseCase,
        std::shared_ptr<abstractions::ILogger>                   logger
    )
        : mLoadConfigUseCase(std::move(loadConfigUseCase))
        , mAnalyzeSystemUseCase(std::move(analyzeSystemUseCase))
        , mLogger(std::move(logger))
        , mStatusText(L"Ready")
        , mWindowTitle(L"WinSetup v1.0")
    {
    }

    std::wstring MainViewModel::GetStatusText()  const { return mStatusText; }
    std::wstring MainViewModel::GetWindowTitle() const { return mWindowTitle; }

    void MainViewModel::SetStatusText(const std::wstring& text) {
        if (mStatusText == text) return;
        mStatusText = text;
        NotifyPropertyChanged(L"StatusText");
    }

    void MainViewModel::SetWindowTitle(const std::wstring& title) {
        if (mWindowTitle == title) return;
        mWindowTitle = title;
        NotifyPropertyChanged(L"WindowTitle");
    }

    std::vector<domain::InstallationType> MainViewModel::GetInstallationTypes() const {
        if (!mConfig) return {};
        return mConfig->GetInstallationTypes();
    }

    std::wstring MainViewModel::GetTypeDescription() const {
        return mTypeDescription;
    }

    void MainViewModel::SetTypeDescription(const std::wstring& key) {
        if (!mConfig) return;
        for (const auto& type : mConfig->GetInstallationTypes()) {
            if (type.name == key) {
                if (mTypeDescription == type.description) return;
                mTypeDescription = type.description;
                NotifyPropertyChanged(L"TypeDescription");
                return;
            }
        }
    }

    bool MainViewModel::GetDataPreservation() const { return mDataPreservation; }
    bool MainViewModel::GetBitlockerEnabled() const { return mBitlockerEnabled; }
    bool MainViewModel::IsInitializing()      const { return mIsInitializing; }
    bool MainViewModel::IsProcessing()        const { return mIsProcessing; }
    bool MainViewModel::IsCompleted()         const { return mIsCompleted; }
    int  MainViewModel::GetProgress()         const { return mProgress; }
    int  MainViewModel::GetRemainingSeconds() const { return static_cast<int>(mRemainingSeconds); }

    void MainViewModel::SetDataPreservation(bool enabled) {
        if (mDataPreservation == enabled) return;
        mDataPreservation = enabled;
        NotifyPropertyChanged(L"DataPreservation");
    }

    void MainViewModel::SetBitlockerEnabled(bool enabled) {
        if (mBitlockerEnabled == enabled) return;
        mBitlockerEnabled = enabled;
        NotifyPropertyChanged(L"BitlockerEnabled");
    }

    void MainViewModel::SetProcessing(bool processing) {
        if (mIsProcessing == processing) return;
        mIsProcessing = processing;
        NotifyPropertyChanged(L"IsProcessing");
    }

    void MainViewModel::TickTimer() {
        if (!mIsProcessing || mIsCompleted) return;

        mElapsedSeconds++;
        if (mTotalSeconds > 0u) {
            mProgress = static_cast<int>(
                static_cast<double>(mElapsedSeconds) / static_cast<double>(mTotalSeconds) * 100.0
                );
            if (mProgress > 100) mProgress = 100;
        }

        mRemainingSeconds = (mTotalSeconds > mElapsedSeconds)
            ? (mTotalSeconds - mElapsedSeconds) : 0u;

        NotifyPropertyChanged(L"Progress");
        NotifyPropertyChanged(L"RemainingSeconds");

        if (mElapsedSeconds >= mTotalSeconds) {
            mIsCompleted = true;
            mIsProcessing = false;
            NotifyPropertyChanged(L"IsCompleted");
            NotifyPropertyChanged(L"IsProcessing");
        }
    }

    domain::Expected<void> MainViewModel::Initialize() {
        mIsInitializing = true;
        SetStatusText(L"Initializing...");
        if (mLogger) mLogger->Info(L"MainViewModel: Initialization started.");

        auto sysResult = RunAnalyzeSystem();
        auto cfgResult = RunLoadConfiguration();

        if (!cfgResult.HasValue()) {
            mIsInitializing = false;
            SetStatusText(L"Failed to load configuration");
            return cfgResult;
        }

        mIsInitializing = false;
        SetStatusText(L"Ready");
        NotifyPropertyChanged(L"InstallationTypes");
        NotifyPropertyChanged(L"RemainingSeconds");

        if (mLogger) mLogger->Info(L"MainViewModel: Initialization completed.");
        return domain::Expected<void>{};
    }

    domain::Expected<void> MainViewModel::RunAnalyzeSystem() {
        if (!mAnalyzeSystemUseCase) {
            return domain::Error(L"AnalyzeSystemUseCase not registered", 0, domain::ErrorCategory::System);
        }
        SetStatusText(L"Reading system information...");

        auto result = mAnalyzeSystemUseCase->Execute();
        if (!result.HasValue()) return result.GetError();

        mAnalysisResult = result.Value();
        return domain::Expected<void>{};
    }

    domain::Expected<void> MainViewModel::RunLoadConfiguration() {
        if (!mLoadConfigUseCase) {
            return domain::Error(L"LoadConfigurationUseCase not registered", 0, domain::ErrorCategory::System);
        }
        SetStatusText(L"Loading configuration...");

        auto result = mLoadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue()) return result.GetError();

        mConfig = result.Value();

        mElapsedSeconds = 0u;
        mTotalSeconds = kDefaultTotalSeconds;
        mRemainingSeconds = kDefaultTotalSeconds;
        mProgress = 0;

        if (mAnalysisResult && mAnalysisResult->systemInfo && mConfig) {
            const std::wstring& model = mAnalysisResult->systemInfo->GetMotherboardModel();
            if (mConfig->HasEstimatedTime(model)) {
                const uint32_t secs = mConfig->GetEstimatedTime(model);
                if (secs > 0u) {
                    mTotalSeconds = secs;
                    mRemainingSeconds = secs;
                }
            }
        }

        return domain::Expected<void>{};
    }

    void MainViewModel::AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) {
        mPropertyChangedHandlers.push_back(std::move(callback));
    }

    void MainViewModel::RemoveAllPropertyChangedHandlers() {
        mPropertyChangedHandlers.clear();
    }

    void MainViewModel::NotifyPropertyChanged(const std::wstring& propertyName) {
        for (const auto& handler : mPropertyChangedHandlers) {
            handler(propertyName);
        }
    }

}
