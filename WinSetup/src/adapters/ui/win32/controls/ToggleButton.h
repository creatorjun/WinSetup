// src/adapters/ui/win32/controls/ToggleButton.h
#pragma once

#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <unordered_map>
#include <vector>

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

        void SetFontSize(int size);
        void SetGroup(int groupId);
        void SetChecked(bool checked);
        void SetEnabled(bool enabled);
        void SetText(const std::wstring& text);

        [[nodiscard]] bool IsChecked() const noexcept { return m_isChecked; }
        [[nodiscard]] bool IsEnabled() const;
        [[nodiscard]] int GetGroup() const noexcept { return m_groupId; }
        [[nodiscard]] std::wstring GetText() const;
        [[nodiscard]] HWND Handle() const noexcept { return m_hwnd; }

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
        void UncheckGroupMembers();
        void InvalidateCache();
        void CleanupCache();
        void UpdateState(bool hovering, bool pressed);

        HWND m_hwnd;
        bool m_isChecked;
        bool m_isHovering;
        bool m_isPressed;
        bool m_wasEnabled;
        int m_groupId;
        adapters::platform::UniqueHandle m_hFont;
        RenderCache m_cache;

        static std::unordered_map<HWND, ToggleButton*> s_instances;
        static std::unordered_map<int, std::vector<ToggleButton*>> s_groups;
    };

}
