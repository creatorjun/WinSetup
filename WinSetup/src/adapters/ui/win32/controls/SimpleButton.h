// src/adapters/ui/win32/controls/SimpleButton.h
#pragma once

#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <unordered_map>

namespace winsetup::adapters::ui {

    class SimpleButton {
    public:
        SimpleButton();
        ~SimpleButton();

        SimpleButton(const SimpleButton&) = delete;
        SimpleButton& operator=(const SimpleButton&) = delete;

        HWND Create(HWND hParent, const std::wstring& text, int x, int y, int width, int height, int id, HINSTANCE hInstance);

        void SetFontSize(int size);
        void SetEnabled(bool enabled);
        void SetText(const std::wstring& text);

        [[nodiscard]] bool IsEnabled() const;
        [[nodiscard]] std::wstring GetText() const;
        [[nodiscard]] HWND Handle() const noexcept { return mHwnd; }

    private:
        struct RenderCache {
            adapters::platform::UniqueHandle hBitmap;
            adapters::platform::UniqueHandle hMemDC;
            int width = 0;
            int height = 0;
            bool isDirty = true;
        };

        static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void DrawButton(HDC hdc);
        void InvalidateCache();
        void CleanupCache();
        void UpdateState(bool hovering, bool pressed);

        HWND mHwnd;
        bool mIsHovering;
        bool mIsPressed;
        bool mWasEnabled;
        adapters::platform::UniqueHandle mHFont;
        RenderCache mCache;

        static std::unordered_map<HWND, SimpleButton*> sInstances;
    };

}
