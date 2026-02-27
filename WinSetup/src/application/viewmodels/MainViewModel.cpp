#include "application/viewmodels/MainViewModel.h"
#include <thread>

namespace winsetup::application {

    MainViewModel::MainViewModel(
        std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase> analyzeSystemUseCase,
        std::shared_ptr<abstractions::IConfigRepository> configRepository,
        std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
        std::shared_ptr<abstractions::IUIDispatcher> dispatcher,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mLoadConfigUseCase(std::move(loadConfigUseCase))
        , mAnalyzeSystemUseCase(std::move(analyzeSystemUseCase))
        , mConfigRepository(std::move(configRepository))
        , mAnalysisRepository(std::move(analysisRepository))
        , mDispatcher(std::move(dispatcher))
        , mLogger(std::move(logger))
        , mStatusText(L"Ready")
        , mWindowTitle(L"WinSetup v1.0")
    {
    }

    std::wstring MainViewModel::GetStatusText() const { return mStatusText; }
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
        if (!mConfigRepository || !mConfigRepository->IsLoaded()) return {};
        auto result = mConfigRepository->GetConfig();
        if (!result.HasValue()) return {};
        return result.Value()->GetInstallationTypes();
    }

    std::wstring MainViewModel::GetTypeDescription() const { return mTypeDescription; }

    void MainViewModel::SetTypeDescription(const std::wstring& key) {
        if (!mConfigRepository || !mConfigRepository->IsLoaded()) return;
        auto result = mConfigRepository->GetConfig();
        if (!result.HasValue()) return;

        for (const auto& type : result.Value()->GetInstallationTypes()) {
            if (type.name != key) continue;

            if (mTypeDescription != type.description) {
                mTypeDescription = type.description;
                NotifyPropertyChanged(L"TypeDescription");
            }

            const bool shouldEnableBitlocker = (key == L"출장용");
            if (mBitlockerEnabled != shouldEnableBitlocker) {
                mBitlockerEnabled = shouldEnableBitlocker;
                NotifyPropertyChanged(L"BitlockerEnabled");
            }
            return;
        }
    }

    bool MainViewModel::GetDataPreservation() const { return mDataPreservation; }
    bool MainViewModel::GetBitlockerEnabled() const { return mBitlockerEnabled; }
    bool MainViewModel::IsInitializing() const { return mIsInitializing; }
    bool MainViewModel::IsProcessing() const { return mIsProcessing; }
    bool MainViewModel::IsCompleted() const { return mIsCompleted; }
    int  MainViewModel::GetProgress() const { return mProgress; }
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
                static_cast<double>(mElapsedSeconds) /
                static_cast<double>(mTotalSeconds) * 100.0);
            if (mProgress > 100) mProgress = 100;
            mRemainingSeconds = (mTotalSeconds > mElapsedSeconds)
                ? (mTotalSeconds - mElapsedSeconds) : 0u;
            NotifyPropertyChanged(L"Progress");
            NotifyPropertyChanged(L"RemainingSeconds");
        }
        if (mElapsedSeconds >= mTotalSeconds) {
            mIsCompleted = true;
            mIsProcessing = false;
            NotifyPropertyChanged(L"IsCompleted");
            NotifyPropertyChanged(L"IsProcessing");
        }
    }

    void MainViewModel::InitializeAsync() {
        if (mIsInitializing) return;
        mIsInitializing = true;
        NotifyPropertyChanged(L"IsInitializing");
        SetStatusText(L"Initializing...");

        if (mLogger) mLogger->Info(L"MainViewModel: InitializeAsync started.");

        auto self = shared_from_this();
        std::thread([self]() {
            self->RunInitializeOnBackground();
            }).detach();
    }

    void MainViewModel::RunInitializeOnBackground() {
        auto sysResult = RunAnalyzeSystem();
        auto cfgResult = RunLoadConfiguration();

        const bool sysOk = sysResult.HasValue();
        const std::wstring sysErrorMsg = sysOk
            ? std::wstring{}
        : sysResult.GetError().GetMessage();

        const bool hasSystemVolume = sysOk && mAnalysisRepository
            && mAnalysisRepository->GetSystemVolume().has_value();
        const bool hasDataVolume = sysOk && mAnalysisRepository
            && mAnalysisRepository->GetDataVolume().has_value();
        const bool canPreserve = hasSystemVolume && hasDataVolume;

        auto dispatcher = mDispatcher;
        auto self = shared_from_this();
        auto capturedCfgResult = std::move(cfgResult);

        dispatcher->Post([self, capturedCfgResult, sysOk, sysErrorMsg, canPreserve]() mutable {
            self->mIsInitializing = false;

            if (!sysOk) {
                self->SetStatusText(sysErrorMsg);
                if (self->mLogger)
                    self->mLogger->Error(L"System analysis failed: " + sysErrorMsg);
                self->NotifyPropertyChanged(L"DisableAllButtons");
                self->NotifyPropertyChanged(L"IsInitializing");
                return;
            }

            if (!capturedCfgResult.HasValue()) {
                self->SetStatusText(L"Failed to load configuration");
                if (self->mLogger)
                    self->mLogger->Error(
                        L"Configuration load failed: " +
                        capturedCfgResult.GetError().GetMessage());
                self->NotifyPropertyChanged(L"DisableAllButtons");
                self->NotifyPropertyChanged(L"IsInitializing");
                return;
            }

            if (canPreserve) {
                self->SetStatusText(L"데이터 보존이 가능합니다.");
                self->NotifyPropertyChanged(L"EnableAllButtons");
            }
            else {
                self->SetStatusText(L"데이터 보존이 불가능합니다.");
                self->NotifyPropertyChanged(L"EnableButtonsWithoutDataPreserve");
            }

            self->NotifyPropertyChanged(L"InstallationTypes");
            self->NotifyPropertyChanged(L"RemainingSeconds");
            self->NotifyPropertyChanged(L"IsInitializing");

            if (self->mLogger)
                self->mLogger->Info(L"MainViewModel: Initialization completed.");
            });
    }

    domain::Expected<void> MainViewModel::RunAnalyzeSystem() {
        if (!mAnalyzeSystemUseCase)
            return domain::Error(L"AnalyzeSystemUseCase not registered", 0,
                domain::ErrorCategory::System);
        SetStatusText(L"Reading system information...");
        auto result = mAnalyzeSystemUseCase->Execute();
        if (!result.HasValue()) return result.GetError();
        return domain::Expected<void>();
    }

    domain::Expected<void> MainViewModel::RunLoadConfiguration() {
        if (!mLoadConfigUseCase)
            return domain::Error(L"LoadConfigurationUseCase not registered", 0,
                domain::ErrorCategory::System);
        if (!mConfigRepository)
            return domain::Error(L"IConfigRepository not registered", 0,
                domain::ErrorCategory::System);
        SetStatusText(L"Loading configuration...");
        auto result = mLoadConfigUseCase->Execute(L"config.ini");
        if (!result.HasValue()) return result.GetError();

        mElapsedSeconds = 0u;
        mTotalSeconds = kDefaultTotalSeconds;
        mRemainingSeconds = kDefaultTotalSeconds;
        mProgress = 0;

        auto cfgResult = mConfigRepository->GetConfig();
        if (!cfgResult.HasValue()) return cfgResult.GetError();
        const auto config = cfgResult.Value();

        if (mAnalysisRepository && mAnalysisRepository->IsLoaded()) {
            auto sysInfoResult = mAnalysisRepository->GetSystemInfo();
            if (sysInfoResult.HasValue()) {
                const std::wstring model = sysInfoResult.Value()->GetMotherboardModel();
                if (config->HasEstimatedTime(model)) {
                    const uint32_t secs = config->GetEstimatedTime(model);
                    if (secs > 0u) {
                        mTotalSeconds = secs;
                        mRemainingSeconds = secs;
                    }
                }
            }
        }
        return domain::Expected<void>();
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
