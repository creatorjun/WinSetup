// src/adapters/ui/win32/Win32ProgressBar.h
#pragma once

#include <Windows.h>
#include <string>

namespace winsetup::adapters::ui {

    class Win32ProgressBar {
    public:
        Win32ProgressBar();
        ~Win32ProgressBar();

        Win32ProgressBar(const Win32ProgressBar&) = delete;
        Win32ProgressBar& operator=(const Win32ProgressBar&) = delete;

        void Create(HWND hParent, HINSTANCE hInstance,
            int x, int y, int width, int height, int id);

        void SetProgress(int percent);
        void SetRemainingSeconds(int seconds);
        void Reset();

        [[nodiscard]] HWND ProgressHandle() const noexcept { return m_hProgressWnd; }
        [[nodiscard]] HWND TimeHandle()     const noexcept { return m_hTimeWnd; }

    private:
        void DrawProgress(HDC hdc) const;
        void UpdateTimeText();

        static LRESULT CALLBACK ProgressSubclassProc(
            HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        HWND m_hProgressWnd = nullptr;
        HWND m_hTimeWnd = nullptr;

        int m_percent = 0;
        int m_remainingSeconds = 0;

        struct RenderCache {
            HBITMAP hBitmap = nullptr;
            HDC     hMemDC = nullptr;
            int     width = 0;
            int     height = 0;
            bool    isDirty = true;
        };

        mutable RenderCache m_cache;

        void InvalidateCache();
        void CleanupCache();

        static constexpr COLORREF COLOR_TRACK = RGB(229, 229, 229);
        static constexpr COLORREF COLOR_FILL = RGB(0, 120, 215);
        static constexpr COLORREF COLOR_BORDER = RGB(172, 172, 172);
        static constexpr COLORREF COLOR_TEXT_BG = RGB(245, 245, 245);
        static constexpr COLORREF COLOR_TEXT_FG = RGB(30, 30, 30);
        static constexpr int      BAR_HEIGHT_MIN = 12;
    };

}
