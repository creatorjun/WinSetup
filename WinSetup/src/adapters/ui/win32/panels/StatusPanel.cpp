// src/adapters/ui/win32/panels/StatusPanel.cpp
#include <adapters/ui/win32/panels/StatusPanel.h>
#include <adapters/platform/win32/core/Win32HandleFactory.h>

namespace winsetup::adapters::ui {

    StatusPanel::StatusPanel()
        : mhParent(nullptr)
        , mx(0), my(0), mwidth(0), mheight(0)
        , mviewModel(nullptr)
    {
    }

    void StatusPanel::Create(HWND hParent, HINSTANCE hInstance, int x, int y, int width, int height) {
        mhParent = hParent;
        mx = x;
        my = y;
        mwidth = width;
        mheight = height;
        EnsureFonts();
    }

    void StatusPanel::EnsureFonts() {
        if (!mFontStatus) {
            mFontStatus = platform::Win32HandleFactory::MakeGdiObject(
                CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI")
            );
        }
        if (!mFontDesc) {
            mFontDesc = platform::Win32HandleFactory::MakeGdiObject(
                CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI")
            );
        }
    }

    void StatusPanel::SetViewModel(std::shared_ptr<abstractions::IMainViewModel> viewModel) {
        mviewModel = std::move(viewModel);
        if (mhParent)
            InvalidateRect(mhParent, nullptr, TRUE);
    }

    void StatusPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!mhParent) return;
        if (propertyName == L"StatusText" || propertyName == L"TypeDescription")
            InvalidateRect(mhParent, nullptr, TRUE);
    }

    void StatusPanel::OnPaint(HDC hdc) {
        EnsureFonts();
        DrawStatusText(hdc);
        DrawTypeDescription(hdc);
    }

    void StatusPanel::DrawStatusText(HDC hdc) const {
        const std::wstring text = mviewModel ? mviewModel->GetStatusText() : L"Ready";
        const RECT rc = { mx, my, mx + mwidth, my + STATUSH };

        HFONT hOldFont = static_cast<HFONT>(
            SelectObject(hdc, platform::Win32HandleFactory::ToWin32Font(mFontStatus))
            );

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextW(hdc, text.c_str(), -1, const_cast<RECT*>(&rc),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldFont);
    }

    void StatusPanel::DrawTypeDescription(HDC hdc) const {
        const std::wstring text = mviewModel ? mviewModel->GetTypeDescription() : L"";
        const bool         empty = text.empty();

        const int descY = my + STATUSH + INNERGAP;
        RECT rc = { mx, descY, mx + mwidth, descY + TYPEDESCH };

        HBRUSH hBg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        HFONT hOldFont = static_cast<HFONT>(
            SelectObject(hdc, platform::Win32HandleFactory::ToWin32Font(mFontDesc))
            );

        const std::wstring displayText = empty ? L"설치 유형을 선택하세요." : text;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, empty ? RGB(160, 160, 160) : RGB(30, 30, 30));
        DrawTextW(hdc, displayText.c_str(), -1, &rc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldFont);
    }

}
