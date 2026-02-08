// src/adapters/ui/win32/controls/ToggleButton.h

#pragma once

#include <windows.h>
#include <basetsd.h>
#include <commctrl.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace winsetup::adapters::ui {

    class ToggleButton {
    public:
        ToggleButton();
        ~ToggleButton();

        ToggleButton(const ToggleButton&) = delete;
        ToggleButton& operator=(const ToggleButton&) = delete;

        static void Initialize(HINSTANCE hInstance);
        static void Cleanup();

        HWND Create(HWND hParent, const std::wstring& text, int x, int y, int width, int height, int id, HINSTANCE hInstance);

        void SetGroup(int groupId);
        void SetChecked(bool checked);
        void Toggle();
        void SetEnabled(bool enabled);
        void SetText(const std::wstring& text);
        void SetFontSize(int size);

        [[nodiscard]] bool IsChecked() const { return isChecked; }
        [[nodiscard]] bool IsEnabled() const;
        [[nodiscard]] HWND Handle() const { return hwnd; }
        [[nodiscard]] int GetGroup() const { return groupId; }
        [[nodiscard]] std::wstring GetText() const;

    private:
        static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void DrawButton(HDC hdc);
        void UncheckGroupMembers();

        HWND hwnd;
        bool isChecked;
        bool isHovering;
        bool isPressed;
        int groupId;
        HFONT hCustomFont;

        static std::unordered_map<HWND, ToggleButton*> instances;
        static std::unordered_map<int, std::vector<ToggleButton*>> groups;
    };

}
