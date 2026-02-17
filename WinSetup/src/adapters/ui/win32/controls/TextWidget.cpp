// src/adapters/ui/win32/controls/TextWidget.cpp
#include <adapters/ui/win32/controls/TextWidget.h>

namespace winsetup::adapters::ui {

    TextWidget::TextWidget()
        : m_style()
        , m_rect{}
        , m_text()
        , m_placeholder()
        , m_font(nullptr)
        , m_fontDirty(true)
    {
    }

    TextWidget::~TextWidget() {
        ReleaseFont();
    }

    void TextWidget::SetStyle(const TextWidgetStyle& style) {
        m_style = style;
        m_fontDirty = true;
    }

    void TextWidget::SetRect(const RECT& rect) {
        m_rect = rect;
    }

    void TextWidget::SetText(const std::wstring& text) {
        m_text = text;
    }

    void TextWidget::SetPlaceholder(const std::wstring& placeholder) {
        m_placeholder = placeholder;
    }

    void TextWidget::InvalidateFontCache() {
        m_fontDirty = true;
    }

    void TextWidget::ReleaseFont() {
        if (m_font) {
            DeleteObject(m_font);
            m_font = nullptr;
        }
        m_fontDirty = true;
    }

    void TextWidget::EnsureFont() const {
        if (!m_fontDirty && m_font) return;

        if (m_font) {
            DeleteObject(m_font);
            m_font = nullptr;
        }

        m_font = CreateFontW(
            m_style.fontSize, 0, 0, 0,
            m_style.fontWeight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI");

        m_fontDirty = false;
    }

    void TextWidget::DrawBorder(HDC hdc, bool condition, POINT from, POINT to) const {
        if (!condition) return;
        HPEN hPen = CreatePen(PS_SOLID, 1, m_style.borderColor);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        MoveToEx(hdc, from.x, from.y, nullptr);
        LineTo(hdc, to.x, to.y);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    void TextWidget::Draw(HDC hdc) const {
        const RECT& r = m_rect;

        if (m_style.drawBackground) {
            HBRUSH hBg = CreateSolidBrush(m_style.bgColor);
            FillRect(hdc, &r, hBg);
            DeleteObject(hBg);
        }

        DrawBorder(hdc, m_style.drawTopBorder,
            { r.left,      r.top },
            { r.right,     r.top });

        DrawBorder(hdc, m_style.drawBottomBorder,
            { r.left,      r.bottom - 1 },
            { r.right,     r.bottom - 1 });

        DrawBorder(hdc, m_style.drawLeftBorder,
            { r.left,      r.top },
            { r.left,      r.bottom });

        DrawBorder(hdc, m_style.drawRightBorder,
            { r.right - 1, r.top },
            { r.right - 1, r.bottom });

        const std::wstring& displayText = m_text.empty() ? m_placeholder : m_text;
        if (displayText.empty()) return;

        EnsureFont();
        HFONT   hFont = m_font
            ? m_font
            : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ hOldFont = SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);

        bool isPlaceholder = m_text.empty() && !m_placeholder.empty();
        SetTextColor(hdc, isPlaceholder ? m_style.placeholderColor : m_style.textColor);

        RECT textRect = {
            r.left + m_style.paddingLeft,
            r.top + m_style.paddingTop,
            r.right - m_style.paddingRight,
            r.bottom - m_style.paddingBottom
        };

        DrawTextW(hdc, displayText.c_str(), -1, &textRect, m_style.dtFormat);

        SelectObject(hdc, hOldFont);
    }

    void TextWidget::Invalidate(HWND hwnd) const {
        if (hwnd)
            InvalidateRect(hwnd, &m_rect, FALSE);
    }

}
