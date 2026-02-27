#include <adapters/ui/win32/controls/TextWidget.h>

namespace winsetup::adapters::ui {

    TextWidget::TextWidget()
        : mStyle()
        , mRect{}
        , mText()
        , mPlaceholder()
        , mFont(nullptr)
        , mFontDirty(true)
        , mBorderPen(nullptr)
        , mPenDirty(true)
    {
    }

    TextWidget::~TextWidget() {
        ReleaseFont();
        ReleasePen();
    }

    void TextWidget::SetStyle(const TextWidgetStyle& style) {
        mStyle = style;
        mFontDirty = true;
        mPenDirty = true;
    }

    void TextWidget::SetRect(const RECT& rect) {
        mRect = rect;
    }

    void TextWidget::SetText(const std::wstring& text) {
        mText = text;
    }

    void TextWidget::SetPlaceholder(const std::wstring& placeholder) {
        mPlaceholder = placeholder;
    }

    void TextWidget::InvalidateFontCache() {
        mFontDirty = true;
    }

    void TextWidget::ReleaseFont() {
        if (mFont) {
            DeleteObject(mFont);
            mFont = nullptr;
        }
        mFontDirty = true;
    }

    void TextWidget::ReleasePen() {
        if (mBorderPen) {
            DeleteObject(mBorderPen);
            mBorderPen = nullptr;
        }
        mPenDirty = true;
    }

    void TextWidget::EnsureFont() const {
        if (!mFontDirty && mFont) return;

        if (mFont) {
            DeleteObject(mFont);
            mFont = nullptr;
        }

        mFont = CreateFontW(
            mStyle.fontSize, 0, 0, 0,
            mStyle.fontWeight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI");

        mFontDirty = false;
    }

    void TextWidget::EnsurePen() const {
        if (!mPenDirty && mBorderPen) return;

        if (mBorderPen) {
            DeleteObject(mBorderPen);
            mBorderPen = nullptr;
        }

        mBorderPen = CreatePen(PS_SOLID, 1, mStyle.borderColor);
        mPenDirty = false;
    }

    void TextWidget::DrawBorder(HDC hdc, bool condition, POINT from, POINT to) const {
        if (!condition) return;
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, mBorderPen));
        MoveToEx(hdc, from.x, from.y, nullptr);
        LineTo(hdc, to.x, to.y);
        SelectObject(hdc, hOldPen);
    }

    void TextWidget::Draw(HDC hdc) const {
        const RECT& r = mRect;

        if (mStyle.drawBackground) {
            HBRUSH hBg = CreateSolidBrush(mStyle.bgColor);
            FillRect(hdc, &r, hBg);
            DeleteObject(hBg);
        }

        const bool hasBorder = mStyle.drawTopBorder || mStyle.drawBottomBorder
            || mStyle.drawLeftBorder || mStyle.drawRightBorder;
        if (hasBorder) {
            EnsurePen();
            DrawBorder(hdc, mStyle.drawTopBorder,
                { r.left,      r.top },
                { r.right,     r.top });
            DrawBorder(hdc, mStyle.drawBottomBorder,
                { r.left,      r.bottom - 1 },
                { r.right,     r.bottom - 1 });
            DrawBorder(hdc, mStyle.drawLeftBorder,
                { r.left,      r.top },
                { r.left,      r.bottom });
            DrawBorder(hdc, mStyle.drawRightBorder,
                { r.right - 1, r.top },
                { r.right - 1, r.bottom });
        }

        const std::wstring& displayText = mText.empty() ? mPlaceholder : mText;
        if (displayText.empty()) return;

        EnsureFont();
        HFONT   hFont = mFont
            ? mFont
            : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ hOldFont = SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);

        bool isPlaceholder = mText.empty() && !mPlaceholder.empty();
        SetTextColor(hdc, isPlaceholder ? mStyle.placeholderColor : mStyle.textColor);

        RECT textRect = {
            r.left + mStyle.paddingLeft,
            r.top + mStyle.paddingTop,
            r.right - mStyle.paddingRight,
            r.bottom - mStyle.paddingBottom
        };

        DrawTextW(hdc, displayText.c_str(), -1, &textRect, mStyle.dtFormat);

        SelectObject(hdc, hOldFont);
    }

    void TextWidget::Invalidate(HWND hwnd) const {
        if (hwnd)
            InvalidateRect(hwnd, &mRect, FALSE);
    }

}
