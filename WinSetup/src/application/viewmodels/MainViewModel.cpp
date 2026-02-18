// src/application/viewmodels/MainViewModel.cpp
#include <application/viewmodels/MainViewModel.h>
#include <domain/primitives/Error.h>

namespace winsetup::application {

    static constexpr uint32_t kDefaultTotalSeconds = 180u;

    MainViewModel::MainViewModel(
        std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     analyzeSystemUseCase,
        std::shared_ptr<abstractions::ILogger>                   logger
    )
        : mLoadConfigUseCase(std::move(loadConfigUseCase))
        , mAnalyzeSystemUseCase(std::move(analyzeSystemUseCase))
        , mLogger(std::move(logger))
        , mConfig(nullptr)
        , mSystemInfo(nullptr)
        , mStatusText(L"Ready")
        , mWindowTitle(L"WinSetup  V 1.0")
        , mTypeDescription(L"")
        , mIsInitializing(false)
        , mIsProcessing(false)
        , mIsCompleted(false)
        , mDataPreservation(false)
        , mBitlockerEnabled(false)
        , mTotalSeconds(kDefaultTotalSeconds)
        , mElapsedSeconds(0u)
        , mRemainingSeconds(kDefaultTotalSeconds)
        , mProgress(0)
    {
    }

    std::wstring MainViewModel::GetStatusText() const { return mStatusText; }

    void MainViewModel::SetStatusText(const std::wstring& text) {
        if (mStatusText != text) {
            mStatusText = text;
            NotifyPropertyChanged(L"StatusText");
        }
    }

    std::wstring MainViewModel::GetWindowTitle() const { return mWindowTitle; }

    void MainViewModel::SetWindowTitle(const std::wstring& title) {
        if (mWindowTitle != title) {
            mWindowTitle = title;
            NotifyPropertyChanged(L"WindowTitle");
        }
    }

    std::vector<domain::InstallationType> MainViewModel::GetInstallationTypes() const {
        if (!mConfig) return {};
        return mConfig->GetInstallationTypes();
    }

    std::wstring MainViewModel::GetTypeDescription() const { return mTypeDescription; }

    void MainViewModel::SetTypeDescription(const std::wstring& key) {
        if (!mConfig) return;
        for (const auto& t : mConfig->GetInstallationTypes()) {
            if (t.name == key) {
                if (mTypeDescription != t.description) {
                    mTypeDescription = t.description;
                    NotifyPropertyChanged(L"TypeDescription");
                }
                return;
            }
        }
    }

    bool MainViewModel::GetDataPreservation() const { return mDataPreservation; }

    void MainViewModel::SetDataPreservation(bool enabled) {
        if (mDataPreservation != enabled) {
            mDataPreservation = enabled;
            NotifyPropertyChanged(L"DataPreservation");
        }
    }

    bool MainViewModel::GetBitlockerEnabled() const { return mBitlockerEnabled; }

    void MainViewModel::SetBitlockerEnabled(bool enabled) {
        if (mBitlockerEnabled != enabled) {
            mBitlockerEnabled = enabled;
            NotifyPropertyChanged(L"BitlockerEnabled");
        }
    }

    void MainViewModel::SetProcessing(bool processing) {
        if (mIsProcessing != processing) {
            mIsProcessing = processing;
            if (mIsProcessing) {
                mElapsedSeconds = 0u;
                mRemainingSeconds = mTotalSeconds;
                mProgress = 0;
                mIsCompleted = false;
            }
            NotifyPropertyChanged(L"IsProcessing");
        }
    }

    int MainViewModel::GetProgress() const { return mProgress; }

    int MainViewModel::GetRemainingSeconds() const { return static_cast<int>(mRemainingSeconds); }

    void MainViewModel::TickTimer() {
        if (!mIsProcessing) return;
        if (mElapsedSeconds < mTotalSeconds) {
            ++mElapsedSeconds;
            mRemainingSeconds = mTotalSeconds - mElapsedSeconds;
            mProgress = static_cast<int>((mElapsedSeconds * 100u) / mTotalSeconds);
            NotifyPropertyChanged(L"Progress");
            NotifyPropertyChanged(L"RemainingSeconds");
            if (mElapsedSeconds >= mTotalSeconds) {
                mIsCompleted = true;
                mIsProcessing = false;
                NotifyPropertyChanged(L"IsCompleted");
                NotifyPropertyChanged(L"IsProcessing");
            }
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
        if (!mAnalyzeSystemUseCase)
            return domain::Error(L"AnalyzeSystemUseCase not registered", 0, domain::ErrorCategory::System);
        SetStatusText(L"Reading system information...");
        auto result = mAnalyzeSystemUseCase->Execute();
        if (!result.HasValue()) return result.GetError();
        mSystemInfo = result.Value();
        return domain::Expected<void>{};
    }

    domain::Expected<void> MainViewModel::RunLoadConfiguration() {
        if (!mLoadConfigUseCase)
            return domain::Error(L"LoadConfigurationUseCase not registered", 0, domain::ErrorCategory::System);
        SetStatusText(L"Loading configuration...");
        auto result = mLoadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue()) return result.GetError();

        mConfig = result.Value();
        mElapsedSeconds = 0u;
        mTotalSeconds = kDefaultTotalSeconds;
        mRemainingSeconds = kDefaultTotalSeconds;
        mProgress = 0;

        if (mSystemInfo) {
            const std::wstring& model = mSystemInfo->GetMotherboardModel();
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
        for (const auto& handler : mPropertyChangedHandlers)
            handler(propertyName);
    }

}
