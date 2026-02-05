#include "StatusTextWidget.h"

namespace winsetup::infrastructure {

    StatusTextWidget::StatusTextWidget() = default;

    StatusTextWidget::~StatusTextWidget() {
        if (hFont_) {
            DeleteObject(hFont_);
        }
        if (hwnd_) {
            DestroyWindow(hwnd_);
        }
    }

    bool StatusTextWidget::Create(
        HWND hParent,
        HINSTANCE hInstance,
        const std::wstring& initialText
    ) {
        if (!hParent) {
            return false;
        }

        hwnd_ = CreateWindowExW(
            WS_EX_STATICEDGE,
            L"STATIC",
            initialText.c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | WS_BORDER,
            0, 0, 0, 0,
            hParent,
            nullptr,
            hInstance,
            nullptr
        );

        if (!hwnd_) {
            return false;
        }

        UpdateFont();

        return true;
    }

    void StatusTextWidget::UpdatePosition(int contentWidth, int contentHeight, int offsetX, int offsetY) {
        if (!hwnd_) {
            return;
        }

        const int height = static_cast<int>(contentHeight * HEIGHT_RATIO);

        SetWindowPos(
            hwnd_,
            nullptr,
            offsetX,
            offsetY,
            contentWidth,
            height,
            SWP_NOZORDER | SWP_NOACTIVATE
        );

        InvalidateRect(hwnd_, nullptr, TRUE);
    }

    void StatusTextWidget::SetText(const std::wstring& text) {
        if (!hwnd_) {
            return;
        }

        SetWindowTextW(hwnd_, text.c_str());
        InvalidateRect(hwnd_, nullptr, TRUE);
    }

    std::wstring StatusTextWidget::GetText() const {
        if (!hwnd_) {
            return L"";
        }

        const int length = GetWindowTextLengthW(hwnd_);
        if (length == 0) {
            return L"";
        }

        std::wstring text(length + 1, L'\0');
        GetWindowTextW(hwnd_, text.data(), length + 1);
        text.resize(length);

        return text;
    }

    void StatusTextWidget::Show() {
        if (hwnd_) {
            ShowWindow(hwnd_, SW_SHOW);
        }
    }

    void StatusTextWidget::Hide() {
        if (hwnd_) {
            ShowWindow(hwnd_, SW_HIDE);
        }
    }

    bool StatusTextWidget::IsVisible() const {
        if (!hwnd_) {
            return false;
        }
        return IsWindowVisible(hwnd_) != FALSE;
    }

    void StatusTextWidget::UpdateFont() {
        if (!hwnd_) {
            return;
        }

        if (hFont_) {
            DeleteObject(hFont_);
        }

        const int fontSize = 18;

        hFont_ = CreateFontW(
            fontSize,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"¸¼Àº °íµñ"
        );

        if (hFont_) {
            SendMessageW(hwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);
        }
    }

}
