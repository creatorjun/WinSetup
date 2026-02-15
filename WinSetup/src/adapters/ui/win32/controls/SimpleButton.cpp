// src/adapters/ui/win32/controls/SimpleButton.cpp
#include "SimpleButton.h"
#include <adapters/platform/win32/core/Win32HandleFactory.h>
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
        constexpr const wchar_t* FONT_NAME = L"Segoe UI";
    }

    std::unordered_map<HWND, SimpleButton*> SimpleButton::s_instances;

    SimpleButton::SimpleButton()
        : m_hwnd(nullptr)
        , m_isHovering(false)
        , m_isPressed(false)
        , m_wasEnabled(true)
        , m_hFont()
    {
    }

    SimpleButton::~SimpleButton() {
        if (m_hwnd) {
            RemoveWindowSubclass(m_hwnd, SimpleButton::SubclassProc, 0);
            s_instances.erase(m_hwnd);
        }
        CleanupCache();
    }

    HWND SimpleButton::Create(HWND hParent, const std::wstring& text, int x, int y, int width, int height, int id, HINSTANCE hInstance) {
        m_hwnd = CreateWindowW(
            L"BUTTON", text.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            x, y, width, height,
            hParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)), hInstance, nullptr
        );

        if (m_hwnd) {
            s_instances[m_hwnd] = this;
            SetWindowSubclass(m_hwnd, SimpleButton::SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
            SetFontSize(13);
        }

        return m_hwnd;
    }

    void SimpleButton::SetFontSize(int size) {
        HFONT hFont = CreateFontW(
            size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_NAME
        );
        m_hFont = platform::Win32HandleFactory::MakeGdiObject(hFont);
        InvalidateCache();
    }

    void SimpleButton::InvalidateCache() {
        m_cache.isDirty = true;
        if (m_hwnd) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void SimpleButton::CleanupCache() {
        m_cache.hBitmap = adapters::platform::UniqueHandle();
        m_cache.hMemDC = adapters::platform::UniqueHandle();
        m_cache.isDirty = true;
    }

    void SimpleButton::UpdateState(bool hovering, bool pressed) {
        bool needUpdate = false;

        if (m_isHovering != hovering) {
            m_isHovering = hovering;
            needUpdate = true;
        }

        if (m_isPressed != pressed) {
            m_isPressed = pressed;
            needUpdate = true;
        }

        if (needUpdate) {
            InvalidateCache();
        }
    }

    LRESULT CALLBACK SimpleButton::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        SimpleButton* pButton = reinterpret_cast<SimpleButton*>(dwRefData);

        if (pButton) {
            switch (uMsg) {
            case WM_MOUSEMOVE:
                if (!pButton->m_isHovering) {
                    pButton->UpdateState(true, pButton->m_isPressed);
                    TRACKMOUSEEVENT tme{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                    TrackMouseEvent(&tme);
                }
                break;

            case WM_MOUSELEAVE:
                if (pButton->m_isHovering || pButton->m_isPressed) {
                    pButton->UpdateState(false, false);
                }
                break;

            case WM_LBUTTONDOWN:
                if (!pButton->m_isPressed) {
                    pButton->UpdateState(pButton->m_isHovering, true);
                }
                break;

            case WM_LBUTTONUP:
                if (pButton->m_isPressed) {
                    pButton->UpdateState(pButton->m_isHovering, false);
                    if (pButton->m_isHovering) {
                        SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), reinterpret_cast<LPARAM>(hWnd));
                    }
                }
                break;

            case WM_ENABLE: {
                bool isEnabled = (wParam != 0);
                if (pButton->m_wasEnabled != isEnabled) {
                    pButton->m_wasEnabled = isEnabled;
                    pButton->InvalidateCache();
                }
                break;
            }

            case WM_SIZE:
                pButton->CleanupCache();
                break;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                RECT rc;
                GetClientRect(hWnd, &rc);

                if (!pButton->m_cache.hMemDC || pButton->m_cache.isDirty ||
                    pButton->m_cache.width != rc.right || pButton->m_cache.height != rc.bottom)
                {
                    if (pButton->m_cache.width != rc.right || pButton->m_cache.height != rc.bottom) {
                        pButton->CleanupCache();
                    }

                    if (!pButton->m_cache.hMemDC) {
                        HDC hMemDC = CreateCompatibleDC(hdc);
                        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);

                        pButton->m_cache.hMemDC = platform::Win32HandleFactory::MakeDC(hMemDC);
                        pButton->m_cache.hBitmap = platform::Win32HandleFactory::MakeGdiObject(hBitmap);

                        SelectObject(platform::Win32HandleFactory::ToWin32DC(pButton->m_cache.hMemDC),
                            platform::Win32HandleFactory::ToWin32Bitmap(pButton->m_cache.hBitmap));
                        pButton->m_cache.width = rc.right;
                        pButton->m_cache.height = rc.bottom;
                    }

                    pButton->DrawButton(platform::Win32HandleFactory::ToWin32DC(pButton->m_cache.hMemDC));
                    pButton->m_cache.isDirty = false;
                }

                BitBlt(hdc, 0, 0, rc.right, rc.bottom,
                    platform::Win32HandleFactory::ToWin32DC(pButton->m_cache.hMemDC),
                    0, 0, SRCCOPY);

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, SimpleButton::SubclassProc, uIdSubclass);
                s_instances.erase(hWnd);
                break;
            }
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void SimpleButton::DrawButton(HDC hdc) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        bool isEnabled = IsWindowEnabled(m_hwnd) != FALSE;

        COLORREF bgColor, textColor;
        if (!isEnabled) {
            bgColor = DISABLED_BG;
            textColor = DISABLED_TEXT;
        }
        else if (m_isPressed) {
            bgColor = CHECKED_BG;
            textColor = CHECKED_TEXT;
        }
        else if (m_isHovering) {
            bgColor = HOVER_BG;
            textColor = NORMAL_TEXT;
        }
        else {
            bgColor = UNCHECKED_BG;
            textColor = NORMAL_TEXT;
        }

        auto hBrush = platform::Win32HandleFactory::MakeGdiObject(CreateSolidBrush(bgColor));
        FillRect(hdc, &rc, platform::Win32HandleFactory::ToWin32Brush(hBrush));

        auto hPen = platform::Win32HandleFactory::MakeGdiObject(CreatePen(PS_SOLID, 1, BORDER_COLOR));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, platform::Win32HandleFactory::ToWin32Pen(hPen)));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, hOldPen);

        std::wstring text = GetText();
        if (!text.empty()) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            HFONT hFont = m_hFont ? platform::Win32HandleFactory::ToWin32Font(m_hFont)
                : reinterpret_cast<HFONT>(SendMessage(m_hwnd, WM_GETFONT, 0, 0));
            HGDIOBJ hOldFont = SelectObject(hdc, hFont);
            DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFont);
        }
    }

    void SimpleButton::SetEnabled(bool enabled) {
        if (!m_hwnd) return;
        EnableWindow(m_hwnd, enabled ? TRUE : FALSE);
    }

    bool SimpleButton::IsEnabled() const {
        if (!m_hwnd) return false;
        return IsWindowEnabled(m_hwnd) != FALSE;
    }

    void SimpleButton::SetText(const std::wstring& text) {
        if (!m_hwnd) return;
        SetWindowTextW(m_hwnd, text.c_str());
        InvalidateCache();
    }

    std::wstring SimpleButton::GetText() const {
        if (!m_hwnd) return L"";
        int len = GetWindowTextLength(m_hwnd);
        if (len == 0) return L"";
        std::vector<wchar_t> buf(static_cast<size_t>(len) + 1);
        GetWindowTextW(m_hwnd, buf.data(), len + 1);
        return std::wstring(buf.data());
    }

}
