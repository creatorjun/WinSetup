// src/application/viewmodels/MainViewModel.cpp
#include <application/viewmodels/MainViewModel.h>
#include <cstdint>

namespace winsetup::application {

    static constexpr uint32_t kDefaultTotalSeconds = 180u;

    MainViewModel::MainViewModel(
        std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     analyzeSystemUseCase,
        std::shared_ptr<abstractions::ILogger>                   logger)
        : mLoadConfigUseCase(std::move(loadConfigUseCase))
        , mAnalyzeSystemUseCase(std::move(analyzeSystemUseCase))
        , mLogger(std::move(logger))
        , mConfig(nullptr)
        , mSystemInfo(nullptr)
        , mStatusText(L"Ready")
        , mWindowTitle(L"WinSetup - PC Reinstallation Tool")
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
        const auto types = mConfig->GetInstallationTypes();
        for (const auto& t : types) {
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
            if (processing) {
                mElapsedSeconds = 0u;
                mRemainingSeconds = mTotalSeconds;
                mProgress = 0;
            }
            else {
                mElapsedSeconds = 0u;
                mRemainingSeconds = 0u;
                mProgress = 0;
            }
            NotifyPropertyChanged(L"IsProcessing");
            NotifyPropertyChanged(L"Progress");
        }
    }

    int MainViewModel::GetProgress()         const { return mProgress; }
    int MainViewModel::GetRemainingSeconds() const { return static_cast<int>(mRemainingSeconds); }

    void MainViewModel::TickTimer() {
        if (!mIsProcessing || mTotalSeconds == 0u) return;
        ++mElapsedSeconds;
        if (mElapsedSeconds >= mTotalSeconds) {
            mElapsedSeconds = mTotalSeconds;
            mRemainingSeconds = 0u;
            mProgress = 100;
        }
        else {
            mRemainingSeconds = mTotalSeconds - mElapsedSeconds;
            mProgress = static_cast<int>(
                static_cast<double>(mElapsedSeconds) /
                static_cast<double>(mTotalSeconds) * 100.0);
        }
        NotifyPropertyChanged(L"Progress");
    }

    domain::Expected<void> MainViewModel::Initialize() {
        mIsInitializing = true;
        SetStatusText(L"Initializing...");
        if (mLogger) mLogger->Info(L"MainViewModel: Initialization started.");

        auto sysResult = RunAnalyzeSystem();
        if (!sysResult.HasValue()) {
            if (mLogger) mLogger->Warning(L"MainViewModel: System analysis failed: " + sysResult.GetError().GetMessage());
        }

        auto cfgResult = RunLoadConfiguration();
        if (!cfgResult.HasValue()) {
            mIsInitializing = false;
            SetStatusText(L"Failed to load configuration");
            return cfgResult;
        }

        mIsInitializing = false;
        SetStatusText(L"Ready");
        NotifyPropertyChanged(L"InstallationTypes");
        if (mLogger) mLogger->Info(L"MainViewModel: Initialization completed.");
        return domain::Expected<void>{};
    }

    domain::Expected<void> MainViewModel::RunAnalyzeSystem() {
        if (!mAnalyzeSystemUseCase)
            return domain::Error(L"AnalyzeSystemUseCase not registered", static_cast<uint32_t>(0), domain::ErrorCategory::System);

        SetStatusText(L"Reading system information...");
        auto result = mAnalyzeSystemUseCase->Execute();
        if (!result.HasValue()) return result.GetError();

        mSystemInfo = result.Value();
        if (mLogger && mSystemInfo)
            mLogger->Info(L"MainViewModel: Motherboard = " + mSystemInfo->GetMotherboardModel());

        return domain::Expected<void>{};
    }

    domain::Expected<void> MainViewModel::RunLoadConfiguration() {
        if (!mLoadConfigUseCase)
            return domain::Error(L"LoadConfigurationUseCase not registered", static_cast<uint32_t>(0), domain::ErrorCategory::System);

        SetStatusText(L"Loading configuration...");
        auto result = mLoadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue()) {
            if (mLogger) mLogger->Error(L"MainViewModel: Failed to load configuration: " + result.GetError().GetMessage());
            return result.GetError();
        }

        mConfig = result.Value();
        mElapsedSeconds = 0u;
        mTotalSeconds = kDefaultTotalSeconds;
        mRemainingSeconds = kDefaultTotalSeconds;
        mProgress = 0;

        if (mSystemInfo) {
            const std::wstring model = mSystemInfo->GetMotherboardModel();
            const uint32_t     estimatedSecs = mConfig->GetEstimatedTime(model);
            if (estimatedSecs > 0u) {
                mTotalSeconds = estimatedSecs;
                mRemainingSeconds = estimatedSecs;
                if (mLogger)
                    mLogger->Info(L"MainViewModel: Estimated time for model [" + model + L"] = " + std::to_wstring(estimatedSecs) + L"s");
            }
            else {
                if (mLogger)
                    mLogger->Warning(L"MainViewModel: No estimated time for model [" + model + L"], using default " + std::to_wstring(kDefaultTotalSeconds) + L"s");
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

} // namespace winsetup::application
