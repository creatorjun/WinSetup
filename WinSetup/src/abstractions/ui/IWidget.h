// src/abstractions/ui/IWidget.h
#pragma once
#include <string>
#include <cstdint>

namespace winsetup::abstractions {

    class IWidget {
    public:
        struct CreateParams {
            void* hParent = nullptr;
            void* hInstance = nullptr;
            int    x = 0;
            int    y = 0;
            int    width = 0;
            int    height = 0;
        };

        virtual ~IWidget() = default;

        virtual void Create(const CreateParams& params) = 0;
        virtual void OnPaint(void* paintContext) = 0;
        virtual bool OnCommand(uintptr_t wParam, uintptr_t lParam) = 0;
        virtual void OnTimer(uintptr_t timerId) = 0;
        virtual void SetEnabled(bool enabled) = 0;
        virtual void OnPropertyChanged(const std::wstring& propertyName) = 0;
        [[nodiscard]] virtual bool IsValid() const noexcept = 0;
    };

} // namespace winsetup::abstractions
