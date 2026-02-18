// src/adapters/ui/win32/Win32ProgressBar.h
#pragma once
#include <Windows.h>
#include <string>

namespace winsetup::adapters::ui {

    class Win32ProgressBar {
    public:
        Win32ProgressBar();
        ~Win32ProgressBar();

        void Create(HWND hParent, HINSTANCE hInstance, int x, int y, int width, int height, int id);
        void SetProgress(int percent);
        void SetRemainingSeconds(int seconds);
        void Reset();

    private:
        void DrawProgress(HDC hdc) const;
        void DrawTime(HDC hdc) const; // [추가]
        void UpdateTimeText();

        static LRESULT CALLBACK ProgressSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        static LRESULT CALLBACK TimeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData); // [추가]

        HWND m_hProgressWnd = nullptr;
        HWND m_hTimeWnd = nullptr;
        int m_percent = 0;
        int m_remainingSeconds = 0;

        struct RenderCache {
            HBITMAP hBitmap = nullptr;
            HDC hMemDC = nullptr;
            int width = 0;
            int height = 0;
            bool isDirty = true;
        } mutable m_cache;

        void InvalidateCache();
        void CleanupCache();

        static constexpr COLORREF COLOR_TRACK = RGB(255, 255, 255);
        static constexpr COLORREF COLOR_FILL = RGB(0, 120, 215);
        static constexpr COLORREF COLOR_BORDER = RGB(172, 172, 172);
        static constexpr COLORREF COLOR_TEXT_BG = RGB(255, 255, 255);
        static constexpr COLORREF COLOR_TEXT_FG = RGB(30, 30, 30);
        static constexpr int BAR_HEIGHT_MIN = 24;
    };
}