// src/application/viewmodels/MainViewModel.h
#pragma once

#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/usecases/ILoadConfigurationUseCase.h>
#include <abstractions/usecases/IAnalyzeSystemUseCase.h>
#include <abstractions/repositories/IConfigRepository.h>
#include <abstractions/repositories/IAnalysisRepository.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <memory>
#include <vector>
#include <string>

namespace winsetup::application {

    class MainViewModel final : public abstractions::IMainViewModel {
    public:
        explicit MainViewModel(
            std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
            std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     analyzeSystemUseCase,
            std::shared_ptr<abstractions::IConfigRepository>         configRepository,
            std::shared_ptr<abstractions::IAnalysisRepository>       analysisRepository,
            std::shared_ptr<abstractions::ILogger>                   logger
        );
        ~MainViewModel() override = default;

        [[nodiscard]] std::wstring GetStatusText()  const override;
        [[nodiscard]] std::wstring GetWindowTitle() const override;
        void SetStatusText(const std::wstring& text)   override;
        void SetWindowTitle(const std::wstring& title) override;

        [[nodiscard]] std::vector<domain::InstallationType> GetInstallationTypes() const override;

        [[nodiscard]] std::wstring GetTypeDescription()   const override;
        void SetTypeDescription(const std::wstring& key)        override;

        [[nodiscard]] bool GetDataPreservation() const override;
        void SetDataPreservation(bool enabled)         override;

        [[nodiscard]] bool GetBitlockerEnabled() const override;
        void SetBitlockerEnabled(bool enabled)         override;

        [[nodiscard]] bool IsInitializing() const override;
        [[nodiscard]] bool IsProcessing()   const override;
        [[nodiscard]] bool IsCompleted()    const override;
        void SetProcessing(bool processing)       override;

        [[nodiscard]] int GetProgress()         const override;
        [[nodiscard]] int GetRemainingSeconds() const override;
        void TickTimer() override;

        domain::Expected<void> Initialize() override;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers() override;

    protected:
        void NotifyPropertyChanged(const std::wstring& propertyName) override;

    private:
        domain::Expected<void> RunAnalyzeSystem();
        domain::Expected<void> RunLoadConfiguration();

        std::shared_ptr<abstractions::ILoadConfigurationUseCase> mLoadConfigUseCase;
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     mAnalyzeSystemUseCase;
        std::shared_ptr<abstractions::IConfigRepository>         mConfigRepository;
        std::shared_ptr<abstractions::IAnalysisRepository>       mAnalysisRepository;
        std::shared_ptr<abstractions::ILogger>                   mLogger;

        std::wstring mStatusText;
        std::wstring mWindowTitle;
        std::wstring mTypeDescription;

        bool     mIsInitializing = false;
        bool     mIsProcessing = false;
        bool     mIsCompleted = false;
        bool     mDataPreservation = false;
        bool     mBitlockerEnabled = false;

        uint32_t mTotalSeconds = 0u;
        uint32_t mElapsedSeconds = 0u;
        uint32_t mRemainingSeconds = 0u;
        int      mProgress = 0;

        static constexpr uint32_t kDefaultTotalSeconds = 180u;

        std::vector<abstractions::PropertyChangedCallback> mPropertyChangedHandlers;
    };

}
