// src/adapters/ui/win32/controls/ToggleButton.cpp
#include "ToggleButton.h"
#include <algorithm>
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

    std::unordered_map<HWND, ToggleButton*> ToggleButton::s_instances;
    std::unordered_map<int, std::vector<ToggleButton*>> ToggleButton::s_groups;

    ToggleButton::ToggleButton()
        : m_hwnd(nullptr)
        , m_isChecked(false)
        , m_isHovering(false)
        , m_isPressed(false)
        , m_wasEnabled(true)
        , m_groupId(-1)
        , m_hFont(nullptr)
    {
    }

    ToggleButton::~ToggleButton() {
        if (m_hwnd) {
            RemoveWindowSubclass(m_hwnd, ToggleButton::SubclassProc, 0);
            s_instances.erase(m_hwnd);
        }
        CleanupCache();
        if (m_hFont) {
            DeleteObject(m_hFont);
        }
        if (m_groupId != -1) {
            auto& group = s_groups[m_groupId];
            auto it = std::find(group.begin(), group.end(), this);
            if (it != group.end()) {
                group.erase(it);
            }
        }
    }

    void ToggleButton::Initialize(HINSTANCE hInstance) {}

    void ToggleButton::Cleanup() {
        s_groups.clear();
    }

    HWND ToggleButton::Create(HWND hParent, const std::wstring& text, int x, int y, int width, int height, int id, HINSTANCE hInstance) {
        m_hwnd = CreateWindowW(
            L"BUTTON", text.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            x, y, width, height,
            hParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)), hInstance, nullptr
        );

        if (m_hwnd) {
            s_instances[m_hwnd] = this;
            SetWindowSubclass(m_hwnd, ToggleButton::SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
            SetFontSize(13);
        }

        return m_hwnd;
    }

    void ToggleButton::SetFontSize(int size) {
        if (m_hFont) {
            DeleteObject(m_hFont);
        }
        m_hFont = CreateFontW(
            size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_NAME
        );
        InvalidateCache();
    }

    void ToggleButton::SetGroup(int groupId) {
        if (m_groupId != -1) {
            auto& oldGroup = s_groups[m_groupId];
            auto it = std::find(oldGroup.begin(), oldGroup.end(), this);
            if (it != oldGroup.end()) {
                oldGroup.erase(it);
            }
        }

        m_groupId = groupId;
        if (groupId != -1) {
            s_groups[groupId].push_back(this);
        }
    }

    void ToggleButton::InvalidateCache() {
        m_cache.isDirty = true;
        if (m_hwnd) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void ToggleButton::CleanupCache() {
        if (m_cache.hBitmap) {
            DeleteObject(m_cache.hBitmap);
            m_cache.hBitmap = nullptr;
        }
        if (m_cache.hMemDC) {
            DeleteDC(m_cache.hMemDC);
            m_cache.hMemDC = nullptr;
        }
        m_cache.isDirty = true;
    }

    void ToggleButton::UpdateState(bool hovering, bool pressed) {
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

    LRESULT CALLBACK ToggleButton::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        ToggleButton* pButton = reinterpret_cast<ToggleButton*>(dwRefData);

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
                        if (pButton->m_groupId != -1) {
                            pButton->UncheckGroupMembers();
                        }
                        pButton->SetChecked(!pButton->m_isChecked);
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
                        pButton->m_cache.hMemDC = CreateCompatibleDC(hdc);
                        pButton->m_cache.hBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                        SelectObject(pButton->m_cache.hMemDC, pButton->m_cache.hBitmap);
                        pButton->m_cache.width = rc.right;
                        pButton->m_cache.height = rc.bottom;
                    }

                    pButton->DrawButton(pButton->m_cache.hMemDC);
                    pButton->m_cache.isDirty = false;
                }

                BitBlt(hdc, 0, 0, rc.right, rc.bottom, pButton->m_cache.hMemDC, 0, 0, SRCCOPY);

                EndPaint(hWnd, &ps);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ToggleButton::SubclassProc, uIdSubclass);
                s_instances.erase(hWnd);
                break;
            }
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void ToggleButton::DrawButton(HDC hdc) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        bool isEnabled = IsWindowEnabled(m_hwnd) != FALSE;

        COLORREF bgColor, textColor;
        if (!isEnabled) {
            bgColor = DISABLED_BG;
            textColor = DISABLED_TEXT;
        }
        else if (m_isChecked) {
            bgColor = CHECKED_BG;
            textColor = CHECKED_TEXT;
        }
        else if (m_isPressed) {
            bgColor = HOVER_BG;
            textColor = NORMAL_TEXT;
        }
        else if (m_isHovering) {
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
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        std::wstring text = GetText();
        if (!text.empty()) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            HFONT hFont = m_hFont ? m_hFont : reinterpret_cast<HFONT>(SendMessage(m_hwnd, WM_GETFONT, 0, 0));
            HGDIOBJ hOldFont = SelectObject(hdc, hFont);
            DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFont);
        }
    }

    void ToggleButton::UncheckGroupMembers() {
        if (m_groupId == -1) return;

        auto it = s_groups.find(m_groupId);
        if (it == s_groups.end()) return;

        for (auto* btn : it->second) {
            if (btn != this && btn->m_isChecked) {
                btn->m_isChecked = false;
                btn->InvalidateCache();
            }
        }
    }

    void ToggleButton::SetChecked(bool checked) {
        if (m_isChecked != checked) {
            m_isChecked = checked;
            InvalidateCache();
        }
    }

    void ToggleButton::SetEnabled(bool enabled) {
        if (!m_hwnd) return;
        EnableWindow(m_hwnd, enabled ? TRUE : FALSE);
    }

    bool ToggleButton::IsEnabled() const {
        if (!m_hwnd) return false;
        return IsWindowEnabled(m_hwnd) != FALSE;
    }

    void ToggleButton::SetText(const std::wstring& text) {
        if (!m_hwnd) return;
        SetWindowTextW(m_hwnd, text.c_str());
        InvalidateCache();
    }

    std::wstring ToggleButton::GetText() const {
        if (!m_hwnd) return L"";
        int len = GetWindowTextLength(m_hwnd);
        if (len == 0) return L"";
        std::vector<wchar_t> buf(static_cast<size_t>(len) + 1);
        GetWindowTextW(m_hwnd, buf.data(), len + 1);
        return std::wstring(buf.data());
    }

}
