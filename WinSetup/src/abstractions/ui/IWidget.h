// src/abstractions/ui/IWidget.h
#pragma once

#include <Windows.h>
#include <string>

namespace winsetup::abstractions {

    class IWidget {
    public:
        virtual ~IWidget() = default;

        virtual void Create(HWND hParent, HINSTANCE hInstance,
            int x, int y, int width, int height) = 0;

        virtual void OnPaint(HDC hdc) = 0;
        virtual bool OnCommand(WPARAM wParam, LPARAM lParam) = 0;
        virtual void OnTimer(UINT_PTR timerId) = 0;

        virtual void SetEnabled(bool enabled) = 0;
        virtual void OnPropertyChanged(const std::wstring& propertyName) = 0;

        [[nodiscard]] virtual bool IsValid() const noexcept = 0;
    };

}
