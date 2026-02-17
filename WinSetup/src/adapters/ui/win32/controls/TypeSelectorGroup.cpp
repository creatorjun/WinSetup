// src/adapters/ui/win32/controls/TypeSelectorGroup.cpp

#include <adapters/ui/win32/controls/TypeSelectorGroup.h>

#undef min
#undef max

namespace winsetup::adapters::ui {

    TypeSelectorGroup::TypeSelectorGroup()
        : m_hParent(nullptr)
        , m_hInstance(nullptr)
        , m_groupId(-1)
        , m_rect{}
        , m_labelFont(nullptr)
        , m_labelFontDirty(true)
        , m_nextButtonId(BTN_ID_BASE)
    {
    }

    TypeSelectorGroup::~TypeSelectorGroup() {
        m_buttons.clear();
        if (m_labelFont) {
            DeleteObject(m_labelFont);
            m_labelFont = nullptr;
        }
    }

    void TypeSelectorGroup::Create(
        HWND hParent, HINSTANCE hInstance,
        const std::wstring& label, int groupId)
    {
        m_hParent = hParent;
        m_hInstance = hInstance;
        m_label = label;
        m_groupId = groupId;
    }

    void TypeSelectorGroup::Rebuild(const std::vector<domain::InstallationType>& types) {
        if (!m_hParent || !m_hInstance) return;

        for (auto& btn : m_buttons)
            if (btn && btn->Handle())
                DestroyWindow(btn->Handle());
        m_buttons.clear();

        m_types = types;
        m_selectedKey = L"";
        m_nextButtonId = BTN_ID_BASE;

        for (size_t i = 0; i < m_types.size(); ++i) {
            auto btn = std::make_unique<ToggleButton>();
            HWND hBtn = btn->Create(
                m_hParent,
                m_types[i].name,
                0, 0,
                BTN_MIN_W, BTN_HEIGHT,
                m_nextButtonId++,
                m_hInstance);

            if (!hBtn) continue;

            btn->SetGroup(m_groupId);
            m_buttons.push_back(std::move(btn));
        }

        if (!m_buttons.empty())
            RecalcButtonRects();

        if (m_hParent)
            InvalidateRect(m_hParent, nullptr, TRUE);
    }

    void TypeSelectorGroup::SetRect(const RECT& rect) {
        m_rect = rect;
        if (!m_buttons.empty())
            RecalcButtonRects();
    }

    void TypeSelectorGroup::SetSelectionChangedCallback(SelectionChangedCallback callback) {
        m_onSelectionChanged = std::move(callback);
    }

    void TypeSelectorGroup::SetEnabled(bool enabled) {
        for (auto& btn : m_buttons) {
            if (btn && btn->Handle())
                EnableWindow(btn->Handle(), enabled ? TRUE : FALSE);
        }
    }

    void TypeSelectorGroup::RecalcButtonRects() {
        if (m_buttons.empty()) return;

        const int areaW = (m_rect.right - m_rect.left) - INNER_PAD_H * 2;
        const int calc = (areaW - BTN_GAP_H * (COLS - 1)) / COLS;
        const int btnW = calc > BTN_MIN_W ? calc : BTN_MIN_W;

        for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i) {
            if (!m_buttons[i]) continue;
            HWND hBtn = m_buttons[i]->Handle();
            if (!hBtn) continue;

            const int col = i % COLS;
            const int row = i / COLS;
            const int x = m_rect.left + INNER_PAD_H + col * (btnW + BTN_GAP_H);
            const int y = m_rect.top + INNER_PAD_TOP + row * (BTN_HEIGHT + BTN_GAP_V);

            SetWindowPos(hBtn, nullptr, x, y, btnW, BTN_HEIGHT,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }

    void TypeSelectorGroup::OnCommand(WPARAM wParam, LPARAM lParam) {
        if (HIWORD(wParam) != BN_CLICKED) return;

        HWND hCtrl = reinterpret_cast<HWND>(lParam);
        if (!hCtrl) return;

        for (size_t i = 0; i < m_buttons.size(); ++i) {
            if (!m_buttons[i]) continue;
            if (m_buttons[i]->Handle() == hCtrl) {
                if (i < m_types.size())
                    m_selectedKey = m_types[i].name;
                if (m_onSelectionChanged)
                    m_onSelectionChanged(m_selectedKey);
                return;
            }
        }
    }

    void TypeSelectorGroup::OnPaint(HDC hdc) const {
        DrawGroupBox(hdc);
    }

    void TypeSelectorGroup::DrawGroupBox(HDC hdc) const {
        if (m_labelFontDirty || !m_labelFont) {
            if (m_labelFont) DeleteObject(m_labelFont);
            m_labelFont = CreateFontW(
                LABEL_FONT_SZ, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            m_labelFontDirty = false;
        }

        HGDIOBJ hOldFont = SelectObject(hdc,
            m_labelFont ? m_labelFont : GetStockObject(DEFAULT_GUI_FONT));

        SIZE labelSize = {};
        GetTextExtentPoint32W(hdc, m_label.c_str(),
            static_cast<int>(m_label.size()), &labelSize);

        const RECT& r = m_rect;
        const int labelX = r.left + INNER_PAD_H;
        const int labelTopY = r.top + LABEL_OFF_Y;
        const int borderTop = r.top + labelSize.cy / 2;

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));

        MoveToEx(hdc, labelX - LABEL_PAD_H, borderTop, nullptr);
        LineTo(hdc, r.left, borderTop);
        LineTo(hdc, r.left, r.bottom - 1);
        LineTo(hdc, r.right - 1, r.bottom - 1);
        LineTo(hdc, r.right - 1, borderTop);
        LineTo(hdc, labelX + labelSize.cx + LABEL_PAD_H, borderTop);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(80, 80, 80));

        RECT labelRect = {
            labelX, labelTopY,
            labelX + labelSize.cx + 1, labelTopY + labelSize.cy + 1
        };
        DrawTextW(hdc, m_label.c_str(), -1, &labelRect,
            DT_LEFT | DT_TOP | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
    }

}
