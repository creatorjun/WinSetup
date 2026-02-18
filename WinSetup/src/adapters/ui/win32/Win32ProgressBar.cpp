// src/adapters/ui/win32/Win32ProgressBar.cpp

#include <adapters/ui/win32/Win32ProgressBar.h>
#include <Windows.h>
#include <commctrl.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace winsetup::adapters::ui {

    Win32ProgressBar::Win32ProgressBar()
        : m_hProgressWnd(nullptr)
        , m_hTimeWnd(nullptr)
        , m_percent(0)
        , m_remainingSeconds(0)
        , m_cache{}
    {
    }

    Win32ProgressBar::~Win32ProgressBar() {
        CleanupCache();
    }

    void Win32ProgressBar::Create(
        HWND hParent, HINSTANCE hInstance,
        int x, int y, int width, int height, int id)
    {
        const int timeW = (width * 30) / 100;
        const int barW = width - timeW - 8;

        m_hProgressWnd = CreateWindowExW(
            0, L"STATIC", nullptr,
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            x, y, barW, height, hParent,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
            hInstance, nullptr);

        if (m_hProgressWnd)
            SetWindowSubclass(m_hProgressWnd, ProgressSubclassProc, 0,
                reinterpret_cast<DWORD_PTR>(this));

        m_hTimeWnd = CreateWindowExW(
            0, L"STATIC", L"예상 시간 : --분 --초",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            x + barW + 8, y, timeW, height, hParent,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id + 1)),
            hInstance, nullptr);

        if (m_hTimeWnd)
            SetWindowSubclass(m_hTimeWnd, TimeSubclassProc, 0,
                reinterpret_cast<DWORD_PTR>(this));
    }

    void Win32ProgressBar::SetProgress(int percent) {
        if (percent < 0)   percent = 0;
        if (percent > 100) percent = 100;
        if (m_percent != percent) {
            m_percent = percent;
            InvalidateCache();
        }
    }

    void Win32ProgressBar::SetRemainingSeconds(int seconds) {
        if (seconds < 0) seconds = 0;
        if (m_remainingSeconds != seconds) {
            m_remainingSeconds = seconds;
            UpdateTimeText();
        }
    }

    void Win32ProgressBar::Reset() {
        m_percent = 0;
        m_remainingSeconds = 0;
        InvalidateCache();
        UpdateTimeText();
    }

    void Win32ProgressBar::InvalidateCache() {
        m_cache.isDirty = true;
        if (m_hProgressWnd)
            InvalidateRect(m_hProgressWnd, nullptr, FALSE);
    }

    void Win32ProgressBar::CleanupCache() {
        if (m_cache.hBitmap) {
            DeleteObject(m_cache.hBitmap);
            m_cache.hBitmap = nullptr;
        }
        if (m_cache.hMemDC) {
            DeleteDC(m_cache.hMemDC);
            m_cache.hMemDC = nullptr;
        }
        m_cache.width = 0;
        m_cache.height = 0;
        m_cache.isDirty = true;
    }

    void Win32ProgressBar::UpdateTimeText() {
        if (!m_hTimeWnd) return;

        wchar_t buf[64];
        if (m_remainingSeconds <= 0)
            swprintf_s(buf, L"예상 시간 : --분 --초");
        else
            swprintf_s(buf, L"예상 시간 : %02d분 %02d초",
                m_remainingSeconds / 60,
                m_remainingSeconds % 60);

        SetWindowTextW(m_hTimeWnd, buf);
        InvalidateRect(m_hTimeWnd, nullptr, TRUE);
    }

    void Win32ProgressBar::DrawProgress(HDC hdc) const {
        if (!m_hProgressWnd) return;

        RECT rc;
        GetClientRect(m_hProgressWnd, &rc);

        if (m_cache.width != rc.right || m_cache.height != rc.bottom)
            const_cast<Win32ProgressBar*>(this)->CleanupCache();

        if (!m_cache.hMemDC) {
            HDC     hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(hMemDC, hBitmap);
            const_cast<Win32ProgressBar*>(this)->m_cache.hMemDC = hMemDC;
            const_cast<Win32ProgressBar*>(this)->m_cache.hBitmap = hBitmap;
            const_cast<Win32ProgressBar*>(this)->m_cache.width = rc.right;
            const_cast<Win32ProgressBar*>(this)->m_cache.height = rc.bottom;
            const_cast<Win32ProgressBar*>(this)->m_cache.isDirty = true;
        }

        if (!m_cache.isDirty) {
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, m_cache.hMemDC, 0, 0, SRCCOPY);
            return;
        }

        HDC memDC = m_cache.hMemDC;

        HBRUSH hBgBrush = CreateSolidBrush(COLOR_TRACK);
        FillRect(memDC, &rc, hBgBrush);
        DeleteObject(hBgBrush);

        const int barY = (rc.bottom - BAR_HEIGHT_MIN) / 2;
        const int barH = BAR_HEIGHT_MIN;
        RECT trackRect = { rc.left, barY, rc.right, barY + barH };

        HPEN hPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(memDC, hPen));
        SelectObject(memDC, GetStockObject(NULL_BRUSH));
        RoundRect(memDC, trackRect.left, trackRect.top, trackRect.right, trackRect.bottom, 4, 4);
        SelectObject(memDC, hOldPen);
        DeleteObject(hPen);

        if (m_percent > 0) {
            const int fillW = static_cast<int>(
                static_cast<double>(trackRect.right - trackRect.left) * m_percent / 100.0);

            if (fillW > 0) {
                RECT fillRect = { trackRect.left, trackRect.top, trackRect.left + fillW, trackRect.bottom };
                HBRUSH hFill = CreateSolidBrush(COLOR_FILL);
                FillRect(memDC, &fillRect, hFill);
                DeleteObject(hFill);
            }
        }

        wchar_t pctBuf[8];
        swprintf_s(pctBuf, L"%d%%", m_percent);
        HFONT hFont = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ hOldFont = SelectObject(memDC, hFont);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, COLOR_TEXT_FG);
        DrawTextW(memDC, pctBuf, -1, const_cast<RECT*>(&rc), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(memDC, hOldFont);
        DeleteObject(hFont);

        const_cast<Win32ProgressBar*>(this)->m_cache.isDirty = false;
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, m_cache.hMemDC, 0, 0, SRCCOPY);
    }

    void Win32ProgressBar::DrawTime(HDC hdc) const {
        if (!m_hTimeWnd) return;
        RECT rc;
        GetClientRect(m_hTimeWnd, &rc);

        HBRUSH hBg = CreateSolidBrush(COLOR_TEXT_BG);
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        wchar_t buf[64];
        GetWindowTextW(m_hTimeWnd, buf, 64);

        HFONT hFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_TEXT_FG);
        DrawTextW(hdc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

    LRESULT CALLBACK Win32ProgressBar::ProgressSubclassProc(
        HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Win32ProgressBar* pBar = reinterpret_cast<Win32ProgressBar*>(dwRefData);
        if (!pBar) return DefSubclassProc(hWnd, uMsg, wParam, lParam);

        switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            pBar->DrawProgress(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_SIZE:       pBar->CleanupCache(); return 0;
        case WM_NCDESTROY:  RemoveWindowSubclass(hWnd, ProgressSubclassProc, uIdSubclass); break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK Win32ProgressBar::TimeSubclassProc(
        HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Win32ProgressBar* pBar = reinterpret_cast<Win32ProgressBar*>(dwRefData);
        if (!pBar) return DefSubclassProc(hWnd, uMsg, wParam, lParam);

        switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            pBar->DrawTime(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_NCDESTROY:  RemoveWindowSubclass(hWnd, TimeSubclassProc, uIdSubclass); break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

} // namespace winsetup::adapters::ui