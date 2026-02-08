// src/adapters/ui/win32/controls/SimpleButton.h

#pragma once

#include <windows.h>
#include <basetsd.h>
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
        [[nodiscard]] HWND Handle() const { return hwnd; }

    private:
        static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void DrawButton(HDC hdc);

        HWND hwnd;
        bool isHovering;
        bool isPressed;
        HFONT hCustomFont;

        static std::unordered_map<HWND, SimpleButton*> instances;
    };

}
