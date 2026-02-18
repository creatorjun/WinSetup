// src/adapters/ui/win32/controls/TextWidget.h
#pragma once

#include <Windows.h>
#include <string>

namespace winsetup::adapters::ui {

    struct TextWidgetStyle {
        int      fontSize = 14;
        int      fontWeight = FW_NORMAL;
        COLORREF textColor = RGB(30, 30, 30);
        COLORREF placeholderColor = RGB(160, 160, 160);
        COLORREF bgColor = RGB(255, 255, 255);
        bool     drawBackground = false;
        bool     drawTopBorder = false;
        bool     drawBottomBorder = false;
        bool     drawLeftBorder = false;
        bool     drawRightBorder = false;
        COLORREF borderColor = RGB(210, 210, 210);
        UINT     dtFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
        int      paddingLeft = 0;
        int      paddingRight = 0;
        int      paddingTop = 0;
        int      paddingBottom = 0;
    };

    class TextWidget {
    public:
        TextWidget();
        ~TextWidget();

        TextWidget(const TextWidget&) = delete;
        TextWidget& operator=(const TextWidget&) = delete;

        void SetStyle(const TextWidgetStyle& style);
        void SetRect(const RECT& rect);
        void SetText(const std::wstring& text);
        void SetPlaceholder(const std::wstring& placeholder);

        [[nodiscard]] const RECT& GetRect()        const noexcept { return mRect; }
        [[nodiscard]] const std::wstring& GetText()        const noexcept { return mText; }
        [[nodiscard]] const std::wstring& GetPlaceholder() const noexcept { return mPlaceholder; }

        void Draw(HDC hdc) const;
        void Invalidate(HWND hwnd) const;
        void InvalidateFontCache();

    private:
        void EnsureFont() const;
        void ReleaseFont();
        void DrawBorder(HDC hdc, bool condition, POINT from, POINT to) const;

        TextWidgetStyle  mStyle;
        RECT             mRect;
        std::wstring     mText;
        std::wstring     mPlaceholder;

        mutable HFONT    mFont;
        mutable bool     mFontDirty;
    };

}
