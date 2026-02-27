// src/abstractions/ui/IMainViewModel.h
#pragma once
#include "IPropertyChanged.h"
#include "domain/primitives/Expected.h"
#include "domain/valueobjects/InstallationType.h"
#include <string>
#include <vector>

namespace winsetup::abstractions {

    class IMainViewModel : public IPropertyChanged {
    public:
        virtual ~IMainViewModel() = default;

        [[nodiscard]] virtual std::wstring GetStatusText() const = 0;
        [[nodiscard]] virtual std::wstring GetWindowTitle() const = 0;
        virtual void SetStatusText(const std::wstring& text) = 0;
        virtual void SetWindowTitle(const std::wstring& title) = 0;

        [[nodiscard]] virtual std::vector<domain::InstallationType> GetInstallationTypes() const = 0;
        [[nodiscard]] virtual std::wstring GetTypeDescription() const = 0;
        virtual void SetTypeDescription(const std::wstring& key) = 0;

        [[nodiscard]] virtual bool GetDataPreservation() const = 0;
        virtual void SetDataPreservation(bool enabled) = 0;

        [[nodiscard]] virtual bool GetBitlockerEnabled() const = 0;
        virtual void SetBitlockerEnabled(bool enabled) = 0;

        [[nodiscard]] virtual bool IsInitializing() const = 0;
        [[nodiscard]] virtual bool IsProcessing() const = 0;
        [[nodiscard]] virtual bool IsCompleted() const = 0;
        virtual void SetProcessing(bool processing) = 0;

        [[nodiscard]] virtual int GetProgress() const = 0;
        [[nodiscard]] virtual int GetRemainingSeconds() const = 0;

        virtual void TickTimer() = 0;
        virtual void InitializeAsync() = 0;
        virtual void StartInstall() = 0;
    };

} // namespace winsetup::abstractions
