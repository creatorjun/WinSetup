// src/adapters/ui/win32/controls/TypeSelectorGroup.cpp
#include <adapters/ui/win32/controls/TypeSelectorGroup.h>
#include <adapters/platform/win32/core/Win32HandleFactory.h>

#undef min
#undef max

namespace winsetup::adapters::ui {

    TypeSelectorGroup::TypeSelectorGroup()
        : mHParent(nullptr)
        , mHInstance(nullptr)
        , mGroupId(-1)
        , mRect{}
        , mNextButtonId(BTNIDBASE)
    {
    }

    TypeSelectorGroup::~TypeSelectorGroup() {
        mButtons.clear();
    }

    void TypeSelectorGroup::Create(
        HWND hParent, HINSTANCE hInstance,
        const std::wstring& label, int groupId)
    {
        mHParent = hParent;
        mHInstance = hInstance;
        mLabel = label;
        mGroupId = groupId;
        EnsureLabelFont();
    }

    void TypeSelectorGroup::EnsureLabelFont() const {
        if (mLabelFont) return;
        mLabelFont = platform::Win32HandleFactory::MakeGdiObject(
            CreateFontW(
                LABELFONTSZ, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI")
        );
    }

    void TypeSelectorGroup::Rebuild(
        const std::vector<domain::InstallationType>& types)
    {
        if (!mHParent || !mHInstance) return;

        for (auto& btn : mButtons) {
            if (!btn) continue;
            HWND hBtn = btn->Handle();
            btn.reset();                          // 1. ~ToggleButton() → SubclassProc 해제
            if (hBtn) DestroyWindow(hBtn);        // 2. HWND 파괴
        }
        mButtons.clear();

        mTypes = types;
        mSelectedKey = L"";
        mNextButtonId = BTNIDBASE;

        for (size_t i = 0; i < mTypes.size(); ++i) {
            auto btn = std::make_unique<ToggleButton>();
            HWND hBtn = btn->Create(
                mHParent, mTypes[i].name,
                0, 0, BTNMINWIDTH, BTNHEIGHT,
                mNextButtonId++, mHInstance);
            if (!hBtn) continue;
            btn->SetGroup(mGroupId);
            mButtons.push_back(std::move(btn));
        }

        if (!mButtons.empty()) RecalcButtonRects();
        if (mHParent) InvalidateRect(mHParent, nullptr, TRUE);
    }

    void TypeSelectorGroup::SetRect(const RECT& rect) {
        mRect = rect;
        if (!mButtons.empty()) RecalcButtonRects();
    }

    void TypeSelectorGroup::SetSelectionChangedCallback(
        SelectionChangedCallback callback)
    {
        mOnSelectionChanged = std::move(callback);
    }

    void TypeSelectorGroup::SetEnabled(bool enabled) {
        for (auto& btn : mButtons)
            if (btn) btn->SetEnabled(enabled);
    }

    void TypeSelectorGroup::OnCommand(WPARAM wParam, LPARAM lParam) {
        if (HIWORD(wParam) != BN_CLICKED) return;

        HWND hCtrl = reinterpret_cast<HWND>(lParam);
        if (!hCtrl) return;

        for (size_t i = 0; i < mButtons.size(); ++i) {
            if (!mButtons[i] || mButtons[i]->Handle() != hCtrl) continue;

            if (i < mTypes.size() && mTypes[i].name == mSelectedKey) {
                mButtons[i]->SetChecked(true);
                return;
            }

            if (i < mTypes.size())
                mSelectedKey = mTypes[i].name;

            if (mOnSelectionChanged)
                mOnSelectionChanged(mSelectedKey);
            return;
        }
    }

    void TypeSelectorGroup::RecalcButtonRects() {
        if (mButtons.empty()) return;

        const int areaW = mRect.right - mRect.left - INNERPADH * 2;
        const int calc = (areaW - BTNGAPH * (COLS - 1)) / COLS;
        const int btnW = calc > BTNMINWIDTH ? calc : BTNMINWIDTH;

        for (int i = 0; i < static_cast<int>(mButtons.size()); ++i) {
            if (!mButtons[i]) continue;
            HWND hBtn = mButtons[i]->Handle();
            if (!hBtn) continue;

            const int col = i % COLS;
            const int row = i / COLS;
            const int x = mRect.left + INNERPADH + col * (btnW + BTNGAPH);
            const int y = mRect.top + INNERPADTOP + row * (BTNHEIGHT + BTNGAPV);

            SetWindowPos(hBtn, nullptr, x, y, btnW, BTNHEIGHT,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }

    void TypeSelectorGroup::DrawGroupBox(HDC hdc) const {
        EnsureLabelFont();

        HGDIOBJ hOldFont = SelectObject(
            hdc, mLabelFont
            ? platform::Win32HandleFactory::ToWin32Font(mLabelFont)
            : GetStockObject(DEFAULT_GUI_FONT));

        SIZE labelSize = {};
        GetTextExtentPoint32W(hdc, mLabel.c_str(),
            static_cast<int>(mLabel.size()), &labelSize);

        const RECT& r = mRect;
        const int   labelX = r.left + INNERPADH;
        const int   labelTopY = r.top + LABELOFFY;
        const int   borderTop = r.top + labelSize.cy / 2;

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));

        MoveToEx(hdc, labelX - LABELPADH, borderTop, nullptr);
        LineTo(hdc, r.left, borderTop);
        LineTo(hdc, r.left, r.bottom - 1);
        LineTo(hdc, r.right - 1, r.bottom - 1);
        LineTo(hdc, r.right - 1, borderTop);
        LineTo(hdc, labelX + labelSize.cx + LABELPADH, borderTop);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(80, 80, 80));

        RECT labelRect = {
            labelX,
            labelTopY,
            labelX + labelSize.cx + 1,
            labelTopY + labelSize.cy + 1
        };
        DrawTextW(hdc, mLabel.c_str(), -1, &labelRect,
            DT_LEFT | DT_TOP | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
    }

    void TypeSelectorGroup::OnPaint(HDC hdc) const {
        DrawGroupBox(hdc);
    }

}
