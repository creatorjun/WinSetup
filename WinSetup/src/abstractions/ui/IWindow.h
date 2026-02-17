// src/abstractions/ui/IWindow.h
#pragma once

#include <memory>

namespace winsetup::abstractions {

    class IWindow {
    public:
        virtual ~IWindow() = default;

        virtual bool Create(void* hInstance, int nCmdShow) = 0;
        virtual void Show() = 0;
        virtual void Hide() = 0;
        virtual void* GetHandle() const noexcept = 0;
    };

}
