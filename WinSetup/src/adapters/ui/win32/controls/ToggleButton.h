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

        [[nodiscard]] bool IsChecked() const noexcept { return mIsChecked; }
        [[nodiscard]] bool IsEnabled() const;
        [[nodiscard]] int  GetGroup() const noexcept { return mGroupId; }
        [[nodiscard]] std::wstring GetText() const;
        [[nodiscard]] HWND Handle() const noexcept { return mHwnd; }

    private:
        struct RenderCache {
            adapters::platform::UniqueHandle hBitmap;
            adapters::platform::UniqueHandle hMemDC;
            int  width = 0;
            int  height = 0;
            bool isDirty = true;
        };

        static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void DrawButton(HDC hdc);
        void UncheckGroupMembers();
        void InvalidateCache();
        void CleanupCache();
        void UpdateState(bool hovering, bool pressed);

        [[nodiscard]] static bool IsInstanceAlive(ToggleButton* ptr) noexcept;

        HWND mHwnd;
        bool mIsChecked;
        bool mIsHovering;
        bool mIsPressed;
        bool mWasEnabled;
        int  mGroupId;
        adapters::platform::UniqueHandle mHFont;
        RenderCache mCache;

        static std::unordered_map<HWND, ToggleButton*>    sInstances;
        static std::unordered_map<int, std::vector<HWND>> sGroups;
    };

}
