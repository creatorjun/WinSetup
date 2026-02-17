// src/application/viewmodels/MainViewModel.h
#pragma once

#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/primitives/Expected.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace winsetup::application {

    class MainViewModel : public abstractions::IMainViewModel {
    public:
        explicit MainViewModel(std::shared_ptr<abstractions::ILogger> logger);
        ~MainViewModel() override = default;

        MainViewModel(const MainViewModel&) = delete;
        MainViewModel& operator=(const MainViewModel&) = delete;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers() override;

        [[nodiscard]] std::wstring GetStatusText() const override;
        void SetStatusText(const std::wstring& text) override;

        [[nodiscard]] std::wstring GetWindowTitle() const override;
        void SetWindowTitle(const std::wstring& title) override;

        [[nodiscard]] bool IsInitializing() const override;
        [[nodiscard]] bool IsProcessing() const override;
        [[nodiscard]] bool IsCompleted() const override;

        domain::Expected<void> Initialize() override;

    protected:
        void NotifyPropertyChanged(const std::wstring& propertyName) override;

    private:
        enum class State {
            Initializing,
            Processing,
            Completed,
            Error
        };

        std::shared_ptr<abstractions::ILogger> mLogger;
        std::vector<abstractions::PropertyChangedCallback> mPropertyChangedHandlers;

        std::wstring mStatusText;
        std::wstring mWindowTitle;
        State mCurrentState;

        mutable std::mutex mMutex;
    };

}
