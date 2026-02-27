// src/application/viewmodels/MainViewModel.h
#pragma once
#include "abstractions/ui/IMainViewModel.h"
#include "abstractions/ui/IUIDispatcher.h"
#include "abstractions/usecases/ILoadConfigurationUseCase.h"
#include "abstractions/usecases/IAnalyzeSystemUseCase.h"
#include "abstractions/usecases/IInstallWindowsUseCase.h"
#include "abstractions/repositories/IConfigRepository.h"
#include "abstractions/repositories/IAnalysisRepository.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include <memory>
#include <vector>
#include <string>

namespace winsetup::application {

    class MainViewModel final
        : public abstractions::IMainViewModel
        , public std::enable_shared_from_this<MainViewModel>
    {
    public:
        explicit MainViewModel(
            std::shared_ptr<abstractions::ILoadConfigurationUseCase> loadConfigUseCase,
            std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     analyzeSystemUseCase,
            std::shared_ptr<abstractions::IInstallWindowsUseCase>    installWindowsUseCase,
            std::shared_ptr<abstractions::IConfigRepository>         configRepository,
            std::shared_ptr<abstractions::IAnalysisRepository>       analysisRepository,
            std::shared_ptr<abstractions::IUIDispatcher>             dispatcher,
            std::shared_ptr<abstractions::ILogger>                   logger);
        ~MainViewModel() override = default;

        [[nodiscard]] std::wstring GetStatusText() const override;
        [[nodiscard]] std::wstring GetWindowTitle() const override;
        [[nodiscard]] std::vector<domain::InstallationType> GetInstallationTypes() const override;
        [[nodiscard]] std::wstring GetTypeDescription() const override;
        [[nodiscard]] bool GetDataPreservation() const override;
        [[nodiscard]] bool GetBitlockerEnabled() const override;
        [[nodiscard]] bool IsInitializing() const override;
        [[nodiscard]] bool IsProcessing() const override;
        [[nodiscard]] bool IsCompleted() const override;
        [[nodiscard]] int GetProgress() const override;
        [[nodiscard]] int GetRemainingSeconds() const override;

        void SetStatusText(const std::wstring& text) override;
        void SetWindowTitle(const std::wstring& title) override;
        void SetTypeDescription(const std::wstring& key) override;
        void SetDataPreservation(bool enabled) override;
        void SetBitlockerEnabled(bool enabled) override;
        void SetProcessing(bool processing) override;
        void TickTimer() override;
        void InitializeAsync() override;
        void StartInstall() override;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers() override;

    private:
        void RunInitializeOnBackground();
        void RunInstallOnBackground();
        domain::Expected<void> RunAnalyzeSystem();
        domain::Expected<void> RunLoadConfiguration();
        void NotifyPropertyChanged(const std::wstring& propertyName);

        std::shared_ptr<abstractions::ILoadConfigurationUseCase> mLoadConfigUseCase;
        std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     mAnalyzeSystemUseCase;
        std::shared_ptr<abstractions::IInstallWindowsUseCase>    mInstallWindowsUseCase;
        std::shared_ptr<abstractions::IConfigRepository>         mConfigRepository;
        std::shared_ptr<abstractions::IAnalysisRepository>       mAnalysisRepository;
        std::shared_ptr<abstractions::IUIDispatcher>             mDispatcher;
        std::shared_ptr<abstractions::ILogger>                   mLogger;

        std::wstring mStatusText;
        std::wstring mWindowTitle;
        std::wstring mTypeDescription;
        bool mDataPreservation{ true };
        bool mBitlockerEnabled{ false };
        bool mIsInitializing{ false };
        bool mIsProcessing{ false };
        bool mIsCompleted{ false };
        int  mProgress{ 0 };
        uint32_t mElapsedSeconds{ 0u };
        uint32_t mTotalSeconds{ kDefaultTotalSeconds };
        uint32_t mRemainingSeconds{ kDefaultTotalSeconds };

        std::vector<abstractions::PropertyChangedCallback> mPropertyChangedHandlers;

        static constexpr uint32_t kDefaultTotalSeconds = 120u;
    };

} // namespace winsetup::application
