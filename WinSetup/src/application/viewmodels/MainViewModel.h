// src/application/viewmodels/MainViewModel.h
#pragma once
#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/usecases/ILoadConfigurationUseCase.h>
#include <abstractions/usecases/IAnalyzeSystemUseCase.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/entities/SetupConfig.h>
#include <domain/entities/SystemInfo.h>
#include <memory>
#include <vector>
#include <string>

namespace winsetup::application {

    class MainViewModel : public abstractions::IMainViewModel {
    public:
        explicit MainViewModel(
            std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
            std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     analyzeSystemUseCase,
            std::shared_ptr<abstractions::ILogger>                   logger);
        ~MainViewModel() override = default;

        [[nodiscard]] std::wstring GetStatusText()  const override;
        void SetStatusText(const std::wstring& text) override;
        [[nodiscard]] std::wstring GetWindowTitle() const override;
        void SetWindowTitle(const std::wstring& title) override;

        [[nodiscard]] std::vector<domain::InstallationType> GetInstallationTypes() const override;
        [[nodiscard]] std::wstring GetTypeDescription() const override;
        void SetTypeDescription(const std::wstring& key) override;

        [[nodiscard]] bool GetDataPreservation() const override;
        void SetDataPreservation(bool enabled)    override;
        [[nodiscard]] bool GetBitlockerEnabled()  const override;
        void SetBitlockerEnabled(bool enabled)    override;

        [[nodiscard]] bool IsInitializing() const override { return mIsInitializing; }
        [[nodiscard]] bool IsProcessing()   const override { return mIsProcessing; }
        [[nodiscard]] bool IsCompleted()    const override { return mIsCompleted; }
        void SetProcessing(bool processing) override;

        [[nodiscard]] int GetProgress()         const override;
        [[nodiscard]] int GetRemainingSeconds() const override;
        void TickTimer() override;

        domain::Expected<void> Initialize() override;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers() override;

        [[nodiscard]] std::shared_ptr<domain::SetupConfig> GetConfig()     const { return mConfig; }
        [[nodiscard]] std::shared_ptr<domain::SystemInfo>  GetSystemInfo() const { return mSystemInfo; }

    protected:
        void NotifyPropertyChanged(const std::wstring& propertyName) override;

    private:
        domain::Expected<void> RunAnalyzeSystem();
        domain::Expected<void> RunLoadConfiguration();

        // ✅ 구체 클래스 → 인터페이스
        std::shared_ptr<abstractions::ILoadConfigurationUseCase> mLoadConfigUseCase;
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     mAnalyzeSystemUseCase;
        std::shared_ptr<abstractions::ILogger>                   mLogger;

        std::shared_ptr<domain::SetupConfig> mConfig;
        std::shared_ptr<domain::SystemInfo>  mSystemInfo;

        std::wstring mStatusText;
        std::wstring mWindowTitle;
        std::wstring mTypeDescription;

        bool     mIsInitializing;
        bool     mIsProcessing;
        bool     mIsCompleted;
        bool     mDataPreservation;
        bool     mBitlockerEnabled;
        uint32_t mTotalSeconds;
        uint32_t mElapsedSeconds;
        uint32_t mRemainingSeconds;
        int      mProgress;

        std::vector<abstractions::PropertyChangedCallback> mPropertyChangedHandlers;
    };

} // namespace winsetup::application
