// src/application/viewmodels/MainViewModel.cpp
#include "MainViewModel.h"

namespace winsetup::application {

    MainViewModel::MainViewModel(std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger))
        , mStatusText(L"초기화 중...")
        , mWindowTitle(L"WinSetup - PC 초기화")
        , mCurrentState(State::Initializing)
    {
        if (mLogger) {
            mLogger->Debug(L"MainViewModel created");
        }
    }

    void MainViewModel::AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPropertyChangedHandlers.push_back(std::move(callback));
    }

    void MainViewModel::RemoveAllPropertyChangedHandlers()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPropertyChangedHandlers.clear();
    }

    void MainViewModel::NotifyPropertyChanged(const std::wstring& propertyName)
    {
        std::vector<abstractions::PropertyChangedCallback> handlersCopy;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            handlersCopy = mPropertyChangedHandlers;
        }

        for (const auto& handler : handlersCopy) {
            handler(propertyName);
        }

        if (mLogger) {
            mLogger->Trace(L"Property changed: " + propertyName);
        }
    }

    std::wstring MainViewModel::GetStatusText() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mStatusText;
    }

    void MainViewModel::SetStatusText(const std::wstring& text)
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mStatusText == text) {
                return;
            }
            mStatusText = text;
        }

        NotifyPropertyChanged(L"StatusText");

        if (mLogger) {
            mLogger->Info(L"Status: " + text);
        }
    }

    std::wstring MainViewModel::GetWindowTitle() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mWindowTitle;
    }

    void MainViewModel::SetWindowTitle(const std::wstring& title)
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mWindowTitle == title) {
                return;
            }
            mWindowTitle = title;
        }

        NotifyPropertyChanged(L"WindowTitle");
    }

    bool MainViewModel::IsInitializing() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCurrentState == State::Initializing;
    }

    bool MainViewModel::IsProcessing() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCurrentState == State::Processing;
    }

    bool MainViewModel::IsCompleted() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCurrentState == State::Completed;
    }

    domain::Expected<void> MainViewModel::Initialize()
    {
        if (mLogger) {
            mLogger->Info(L"MainViewModel initializing...");
        }

        {
            std::lock_guard<std::mutex> lock(mMutex);
            mCurrentState = State::Processing;
        }

        SetStatusText(L"시스템 준비 완료");

        if (mLogger) {
            mLogger->Info(L"MainViewModel initialized successfully");
        }

        return domain::Expected<void>();
    }

}
