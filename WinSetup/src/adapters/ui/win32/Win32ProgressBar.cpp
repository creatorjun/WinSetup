#include <adapters/ui/win32/Win32ProgressBar.h>
#include <adapters/platform/win32/core/Win32HandleFactory.h>
#include <Windows.h>
#include <commctrl.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace winsetup::adapters::ui {

    Win32ProgressBar::Win32ProgressBar()
        : mhProgressWnd(nullptr)
        , mhTimeWnd(nullptr)
        , mPercent(0)
        , mRemainingSeconds(0)
        , mCache{}
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

        mhProgressWnd = CreateWindowExW(
            0, L"STATIC", nullptr,
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            x, y, barW, height, hParent,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
            hInstance, nullptr);

        if (mhProgressWnd)
            SetWindowSubclass(mhProgressWnd, ProgressSubclassProc, 0,
                reinterpret_cast<DWORD_PTR>(this));

        mhTimeWnd = CreateWindowExW(
            0, L"STATIC", L"예상 시간 : --분 --초",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            x + barW + 8, y, timeW, height, hParent,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id + 1)),
            hInstance, nullptr);

        if (mhTimeWnd)
            SetWindowSubclass(mhTimeWnd, TimeSubclassProc, 0,
                reinterpret_cast<DWORD_PTR>(this));

        EnsureFonts();
        EnsureBrushes();
    }

    void Win32ProgressBar::EnsureFonts() {
        if (!mFontProgress) {
            mFontProgress = platform::Win32HandleFactory::MakeGdiObject(
                CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI")
            );
        }
        if (!mFontTime) {
            mFontTime = platform::Win32HandleFactory::MakeGdiObject(
                CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI")
            );
        }
    }

    void Win32ProgressBar::EnsureBrushes() {
        if (!mBrushBg) {
            mBrushBg = platform::Win32HandleFactory::MakeGdiObject(
                CreateSolidBrush(COLOR_TRACK)
            );
        }
        if (!mBrushFill) {
            mBrushFill = platform::Win32HandleFactory::MakeGdiObject(
                CreateSolidBrush(COLOR_FILL)
            );
        }
        if (!mPenBorder) {
            mPenBorder = platform::Win32HandleFactory::MakeGdiObject(
                CreatePen(PS_SOLID, 1, COLOR_BORDER)
            );
        }
    }

    void Win32ProgressBar::SetProgress(int percent) {
        if (percent < 0)   percent = 0;
        if (percent > 100) percent = 100;
        if (mPercent != percent) {
            mPercent = percent;
            InvalidateCache();
        }
    }

    void Win32ProgressBar::SetRemainingSeconds(int seconds) {
        if (seconds < 0) seconds = 0;
        if (mRemainingSeconds != seconds) {
            mRemainingSeconds = seconds;
            UpdateTimeText();
        }
    }

    void Win32ProgressBar::Reset() {
        mPercent = 0;
        mRemainingSeconds = 0;
        InvalidateCache();
        UpdateTimeText();
    }

    void Win32ProgressBar::InvalidateCache() {
        mCache.isDirty = true;
        if (mhProgressWnd)
            InvalidateRect(mhProgressWnd, nullptr, FALSE);
    }

    void Win32ProgressBar::CleanupCache() {
        if (mCache.hBitmap) {
            DeleteObject(mCache.hBitmap);
            mCache.hBitmap = nullptr;
        }
        if (mCache.hMemDC) {
            DeleteDC(mCache.hMemDC);
            mCache.hMemDC = nullptr;
        }
        mCache.width = 0;
        mCache.height = 0;
        mCache.isDirty = true;
    }

    void Win32ProgressBar::UpdateTimeText() {
        if (!mhTimeWnd) return;

        wchar_t buf[64];
        if (mRemainingSeconds <= 0)
            swprintf_s(buf, L"예상 시간 : --분 --초");
        else
            swprintf_s(buf, L"예상 시간 : %02d분 %02d초",
                mRemainingSeconds / 60,
                mRemainingSeconds % 60);

        SetWindowTextW(mhTimeWnd, buf);
        InvalidateRect(mhTimeWnd, nullptr, TRUE);
    }

    void Win32ProgressBar::DrawProgress(HDC hdc) const {
        if (!mhProgressWnd) return;

        RECT rc;
        GetClientRect(mhProgressWnd, &rc);

        if (mCache.width != rc.right || mCache.height != rc.bottom)
            const_cast<Win32ProgressBar*>(this)->CleanupCache();

        if (!mCache.hMemDC) {
            HDC     hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(hMemDC, hBitmap);
            auto* self = const_cast<Win32ProgressBar*>(this);
            self->mCache.hMemDC = hMemDC;
            self->mCache.hBitmap = hBitmap;
            self->mCache.width = rc.right;
            self->mCache.height = rc.bottom;
            self->mCache.isDirty = true;
        }

        if (!mCache.isDirty) {
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mCache.hMemDC, 0, 0, SRCCOPY);
            return;
        }

        HDC memDC = mCache.hMemDC;

        FillRect(memDC, &rc, platform::Win32HandleFactory::ToWin32Brush(mBrushBg));

        const int barY = (rc.bottom - BAR_HEIGHT_MIN) / 2;
        RECT trackRect = { rc.left, barY, rc.right, barY + BAR_HEIGHT_MIN };

        HPEN hOldPen = static_cast<HPEN>(
            SelectObject(memDC, platform::Win32HandleFactory::ToWin32Pen(mPenBorder)));
        SelectObject(memDC, GetStockObject(NULL_BRUSH));
        RoundRect(memDC, trackRect.left, trackRect.top,
            trackRect.right, trackRect.bottom, 4, 4);
        SelectObject(memDC, hOldPen);

        if (mPercent > 0) {
            const int fillW = static_cast<int>(
                static_cast<double>(trackRect.right - trackRect.left)
                * mPercent / 100.0);
            if (fillW > 0) {
                RECT fillRect = {
                    trackRect.left, trackRect.top,
                    trackRect.left + fillW, trackRect.bottom
                };
                FillRect(memDC, &fillRect,
                    platform::Win32HandleFactory::ToWin32Brush(mBrushFill));
            }
        }

        wchar_t pctBuf[8];
        swprintf_s(pctBuf, L"%d%%", mPercent);

        HGDIOBJ hOldFont = SelectObject(
            memDC, platform::Win32HandleFactory::ToWin32Font(mFontProgress));
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, COLOR_TEXT_FG);
        DrawTextW(memDC, pctBuf, -1, const_cast<RECT*>(&rc),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(memDC, hOldFont);

        const_cast<Win32ProgressBar*>(this)->mCache.isDirty = false;
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mCache.hMemDC, 0, 0, SRCCOPY);
    }

    void Win32ProgressBar::DrawTime(HDC hdc) const {
        if (!mhTimeWnd) return;

        RECT rc;
        GetClientRect(mhTimeWnd, &rc);

        FillRect(hdc, &rc, platform::Win32HandleFactory::ToWin32Brush(mBrushBg));

        wchar_t buf[64];
        GetWindowTextW(mhTimeWnd, buf, 64);

        HGDIOBJ hOldFont = SelectObject(
            hdc, platform::Win32HandleFactory::ToWin32Font(mFontTime));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_TEXT_FG);
        DrawTextW(hdc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, hOldFont);
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
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ProgressSubclassProc, uIdSubclass);
            break;
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
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, TimeSubclassProc, uIdSubclass);
            break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

}
