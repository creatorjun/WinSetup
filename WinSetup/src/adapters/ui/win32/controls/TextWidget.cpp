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

    void TextWidget::Draw(HDC hdc) const {
        const RECT& drawRect = m_rect;

        if (m_style.drawBackground) {
            HBRUSH hBg = CreateSolidBrush(m_style.bgColor);
            FillRect(hdc, &drawRect, hBg);
            DeleteObject(hBg);
        }

        if (m_style.drawTopBorder) {
            HPEN hPen = CreatePen(PS_SOLID, 1, m_style.borderColor);
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
            MoveToEx(hdc, drawRect.left, drawRect.top, nullptr);
            LineTo(hdc, drawRect.right, drawRect.top);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }

        if (m_style.drawBottomBorder) {
            HPEN hPen = CreatePen(PS_SOLID, 1, m_style.borderColor);
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
            MoveToEx(hdc, drawRect.left, drawRect.bottom - 1, nullptr);
            LineTo(hdc, drawRect.right, drawRect.bottom - 1);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }

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
            drawRect.left + m_style.paddingLeft,
            drawRect.top + m_style.paddingTop,
            drawRect.right - m_style.paddingRight,
            drawRect.bottom - m_style.paddingBottom
        };

        DrawTextW(hdc, displayText.c_str(), -1, &textRect, m_style.dtFormat);

        SelectObject(hdc, hOldFont);
    }

    void TextWidget::Invalidate(HWND hwnd) const {
        if (hwnd)
            InvalidateRect(hwnd, &m_rect, FALSE);
    }

}
