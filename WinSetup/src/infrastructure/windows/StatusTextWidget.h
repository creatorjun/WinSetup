#pragma once

#include <Windows.h>
#include <string>

namespace winsetup::infrastructure {

    class StatusTextWidget {
    public:
        StatusTextWidget();
        ~StatusTextWidget();

        StatusTextWidget(const StatusTextWidget&) = delete;
        StatusTextWidget& operator=(const StatusTextWidget&) = delete;
        StatusTextWidget(StatusTextWidget&&) = delete;
        StatusTextWidget& operator=(StatusTextWidget&&) = delete;

        [[nodiscard]] bool Create(
            HWND hParent,
            HINSTANCE hInstance,
            const std::wstring& initialText = L"시스템 분석중입니다."
        );

        void UpdatePosition(int contentWidth, int contentHeight, int offsetX, int offsetY);
        void SetText(const std::wstring& text);
        [[nodiscard]] std::wstring GetText() const;

        void Show();
        void Hide();
        [[nodiscard]] bool IsVisible() const;

        [[nodiscard]] HWND GetHandle() const noexcept { return hwnd_; }

    private:
        void UpdateFont();

        HWND hwnd_{ nullptr };
        HFONT hFont_{ nullptr };

        static constexpr double HEIGHT_RATIO = 0.1;
    };

}
