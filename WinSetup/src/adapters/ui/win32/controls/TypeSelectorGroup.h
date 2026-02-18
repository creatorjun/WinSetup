// src/adapters/ui/win32/controls/TypeSelectorGroup.h
#pragma once

#include <adapters/ui/win32/controls/ToggleButton.h>
#include <domain/entities/SetupConfig.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace winsetup::adapters::ui {

    class TypeSelectorGroup {
    public:
        using SelectionChangedCallback =
            std::function<void(const std::wstring& selectedKey)>;

        TypeSelectorGroup();
        ~TypeSelectorGroup();

        TypeSelectorGroup(const TypeSelectorGroup&) = delete;
        TypeSelectorGroup& operator=(const TypeSelectorGroup&) = delete;

        void Create(HWND hParent, HINSTANCE hInstance,
            const std::wstring& label, int groupId);

        void Rebuild(const std::vector<domain::InstallationType>& types);
        void SetRect(const RECT& rect);
        void SetSelectionChangedCallback(SelectionChangedCallback callback);
        void SetEnabled(bool enabled);

        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnPaint(HDC hdc) const;

        [[nodiscard]] const std::wstring& GetSelectedKey() const noexcept { return mSelectedKey; }
        [[nodiscard]] const RECT& GetRect()        const noexcept { return mRect; }
        [[nodiscard]] bool                IsReady()        const noexcept { return !mTypes.empty(); }

    private:
        void RecalcButtonRects();
        void DrawGroupBox(HDC hdc) const;

        HWND      mHParent = nullptr;
        HINSTANCE mHInstance = nullptr;

        std::wstring                               mLabel;
        std::vector<domain::InstallationType>      mTypes;
        std::vector<std::unique_ptr<ToggleButton>> mButtons;
        std::wstring                               mSelectedKey;
        SelectionChangedCallback                   mOnSelectionChanged;

        int  mGroupId = -1;
        RECT mRect = {};

        mutable HFONT mLabelFont = nullptr;
        mutable bool  mLabelFontDirty = true;

        int mNextButtonId = BTNIDBASE;

        static constexpr int COLS = 2;
        static constexpr int BTNHEIGHT = 32;
        static constexpr int BTNMINWIDTH = 80;
        static constexpr int BTNGAPH = 8;
        static constexpr int BTNGAPV = 8;
        static constexpr int INNERPADH = 12;
        static constexpr int INNERPADTOP = 28;
        static constexpr int INNERPADBOT = 12;
        static constexpr int LABELFONTSZ = 12;
        static constexpr int LABELOFFY = 3;
        static constexpr int LABELPADH = 6;
        static constexpr int BTNIDBASE = 3000;
    };

}
