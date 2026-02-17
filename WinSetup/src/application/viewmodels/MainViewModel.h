// src/application/viewmodels/MainViewModel.h
#pragma once

#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <application/usecases/system/LoadConfigurationUseCase.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <vector>
#include <string>

namespace winsetup::application {

    class MainViewModel : public abstractions::IMainViewModel {
    public:
        explicit MainViewModel(
            std::shared_ptr<LoadConfigurationUseCase> loadConfigUseCase,
            std::shared_ptr<abstractions::ILogger> logger
        );
        ~MainViewModel() override = default;

        [[nodiscard]] std::wstring GetStatusText() const override;
        void SetStatusText(const std::wstring& text) override;

        [[nodiscard]] std::wstring GetWindowTitle() const override;
        void SetWindowTitle(const std::wstring& title) override;

        [[nodiscard]] bool IsInitializing() const override { return mIsInitializing; }
        [[nodiscard]] bool IsProcessing()  const override { return mIsProcessing; }
        [[nodiscard]] bool IsCompleted()   const override { return mIsCompleted; }

        domain::Expected<void> Initialize() override;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers() override;

        [[nodiscard]] std::shared_ptr<domain::SetupConfig> GetConfig() const { return mConfig; }

    protected:
        void NotifyPropertyChanged(const std::wstring& propertyName) override;

    private:
        domain::Expected<void> LoadConfiguration();

        std::shared_ptr<LoadConfigurationUseCase> mLoadConfigUseCase;
        std::shared_ptr<abstractions::ILogger>    mLogger;
        std::shared_ptr<domain::SetupConfig>      mConfig;

        std::wstring mStatusText;
        std::wstring mWindowTitle;

        bool mIsInitializing{ false };
        bool mIsProcessing{ false };
        bool mIsCompleted{ false };

        std::vector<abstractions::PropertyChangedCallback> mPropertyChangedHandlers;
    };

}
