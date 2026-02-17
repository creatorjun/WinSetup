// src/abstractions/ui/IWindow.h
#pragma once

namespace winsetup::abstractions {

    class IWindow {
    public:
        virtual ~IWindow() = default;

        virtual void Show() = 0;
        virtual void Hide() = 0;

        [[nodiscard]] virtual bool IsValid() const noexcept = 0;
        [[nodiscard]] virtual bool RunMessageLoop() = 0;
    };

}
