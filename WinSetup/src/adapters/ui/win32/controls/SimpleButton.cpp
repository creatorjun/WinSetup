// src/adapters/ui/win32/controls/SimpleButton.cpp

#include "SimpleButton.h"
#include <windowsx.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace winsetup::adapters::ui {

    namespace {
        constexpr COLORREF CHECKED_BG = RGB(0, 120, 215);
        constexpr COLORREF UNCHECKED_BG = RGB(225, 225, 225);
        constexpr COLORREF HOVER_BG = RGB(229, 241, 251);
        constexpr COLORREF DISABLED_BG = RGB(204, 204, 204);
        constexpr COLORREF CHECKED_TEXT = RGB(255, 255, 255);
        constexpr COLORREF NORMAL_TEXT = RGB(0, 0, 0);
        constexpr COLORREF DISABLED_TEXT = RGB(160, 160, 160);
        constexpr COLORREF BORDER_COLOR = RGB(172, 172, 172);
        constexpr const wchar_t* FONT_NAME = L"¸¼Àº °íµñ";
    }

    std::unordered_map<HWND, SimpleButton*> SimpleButton::instances;

    SimpleButton::SimpleButton()
        : hwnd(nullptr)
        , isHovering(false)
        , isPressed(false)
        , hCustomFont(nullptr)
    {
    }

    SimpleButton::~SimpleButton() {
        if (hwnd) {
            RemoveWindowSubclass(hwnd, SimpleButton::SubclassProc, 0);
            instances.erase(hwnd);
        }
        if (hCustomFont) {
            DeleteObject(hCustomFont);
        }
    }

    HWND SimpleButton::Create(HWND hParent, const std::wstring& text, int x, int y, int width, int height, int id, HINSTANCE hInstance) {
        hwnd = CreateWindowW(
            L"BUTTON", text.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            x, y, width, height,
            hParent, (HMENU)(UINT_PTR)id, hInstance, nullptr
        );

        if (hwnd) {
            instances[hwnd] = this;
            SetWindowSubclass(hwnd, SimpleButton::SubclassProc, 0, (DWORD_PTR)this);
            SetFontSize(13);
        }

        return hwnd;
    }

    void SimpleButton::SetFontSize(int size) {
        if (hCustomFont) {
            DeleteObject(hCustomFont);
        }
        hCustomFont = CreateFont(
            size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_NAME
        );
        if (hwnd) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    LRESULT CALLBACK SimpleButton::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        SimpleButton* pButton = reinterpret_cast<SimpleButton*>(dwRefData);

        if (pButton) {
            switch (uMsg) {
            case WM_MOUSEMOVE:
                if (!pButton->isHovering) {
                    pButton->isHovering = true;
                    TRACKMOUSEEVENT tme{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                    TrackMouseEvent(&tme);
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                break;

            case WM_MOUSELEAVE:
                pButton->isHovering = false;
                pButton->isPressed = false;
                InvalidateRect(hWnd, nullptr, FALSE);
                break;

            case WM_LBUTTONDOWN:
                pButton->isPressed = true;
                InvalidateRect(hWnd, nullptr, FALSE);
                break;

            case WM_LBUTTONUP:
                if (pButton->isPressed) {
                    pButton->isPressed = false;
                    if (pButton->isHovering) {
                        SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
                    }
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                break;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                RECT rc;
                GetClientRect(hWnd, &rc);

                HDC hMemDC = CreateCompatibleDC(hdc);
                HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

                pButton->DrawButton(hMemDC);

                BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMemDC, 0, 0, SRCCOPY);

                SelectObject(hMemDC, hOldBitmap);
                DeleteObject(hBitmap);
                DeleteDC(hMemDC);

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, SimpleButton::SubclassProc, uIdSubclass);
                instances.erase(hWnd);
                break;
            }
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void SimpleButton::DrawButton(HDC hdc) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        bool isEnabled = IsEnabled();

        COLORREF bgColor, textColor;
        if (!isEnabled) {
            bgColor = DISABLED_BG;
            textColor = DISABLED_TEXT;
        }
        else if (isPressed) {
            bgColor = CHECKED_BG;
            textColor = CHECKED_TEXT;
        }
        else if (isHovering) {
            bgColor = HOVER_BG;
            textColor = NORMAL_TEXT;
        }
        else {
            bgColor = UNCHECKED_BG;
            textColor = NORMAL_TEXT;
        }

        HBRUSH hBrush = CreateSolidBrush(bgColor);
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        HPEN hPen = CreatePen(PS_SOLID, 1, BORDER_COLOR);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        std::wstring text = GetText();
        if (!text.empty()) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            HFONT hFont = hCustomFont ? hCustomFont : (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
            HGDIOBJ hOldFont = SelectObject(hdc, hFont);
            DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFont);
        }
    }

    void SimpleButton::SetEnabled(bool enabled) {
        if (!hwnd) return;
        EnableWindow(hwnd, enabled ? TRUE : FALSE);
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    bool SimpleButton::IsEnabled() const {
        if (!hwnd) return false;
        return IsWindowEnabled(hwnd) != FALSE;
    }

    void SimpleButton::SetText(const std::wstring& text) {
        if (!hwnd) return;
        SetWindowTextW(hwnd, text.c_str());
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    std::wstring SimpleButton::GetText() const {
        if (!hwnd) return L"";
        int len = GetWindowTextLength(hwnd);
        if (len == 0) return L"";
        std::vector<wchar_t> buf(len + 1);
        GetWindowTextW(hwnd, buf.data(), len + 1);
        return std::wstring(buf.data());
    }

}
