// src/abstractions/ui/IMainViewModel.h
#pragma once

#include <abstractions/ui/IPropertyChanged.h>
#include <domain/primitives/Expected.h>
#include <string>
#include <memory>

namespace winsetup::abstractions {

    class IMainViewModel : public IPropertyChanged {
    public:
        virtual ~IMainViewModel() = default;

        [[nodiscard]] virtual std::wstring GetStatusText() const = 0;
        virtual void SetStatusText(const std::wstring& text) = 0;

        [[nodiscard]] virtual std::wstring GetWindowTitle() const = 0;
        virtual void SetWindowTitle(const std::wstring& title) = 0;

        [[nodiscard]] virtual bool IsInitializing() const = 0;
        [[nodiscard]] virtual bool IsProcessing() const = 0;
        [[nodiscard]] virtual bool IsCompleted() const = 0;

        virtual domain::Expected<void> Initialize() = 0;
    };

}
