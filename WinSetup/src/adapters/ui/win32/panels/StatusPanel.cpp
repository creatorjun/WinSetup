// src/adapters/ui/win32/panels/StatusPanel.cpp

#include <adapters/ui/win32/panels/StatusPanel.h>

namespace winsetup::adapters::ui {

    StatusPanel::StatusPanel()
        : m_hParent(nullptr)
        , m_x(0), m_y(0), m_width(0), m_height(0)
        , m_statusText(L"Ready")
        , m_typeDescription(L"")
        , m_viewModel(nullptr)
    {
    }

    void StatusPanel::Create(
        HWND hParent, HINSTANCE /*hInstance*/,
        int x, int y, int width, int height)
    {
        m_hParent = hParent;
        m_x = x;
        m_y = y;
        m_width = width;
        m_height = height;

        if (m_viewModel) {
            m_statusText = m_viewModel->GetStatusText();
            m_typeDescription = m_viewModel->GetTypeDescription();
        }
    }

    void StatusPanel::SetViewModel(
        std::shared_ptr<abstractions::IMainViewModel> viewModel)
    {
        m_viewModel = std::move(viewModel);
        if (m_viewModel) {
            m_statusText = m_viewModel->GetStatusText();
            m_typeDescription = m_viewModel->GetTypeDescription();
        }
    }

    void StatusPanel::OnPropertyChanged(const std::wstring& propertyName) {
        if (!m_viewModel || !m_hParent) return;

        if (propertyName == L"StatusText") {
            m_statusText = m_viewModel->GetStatusText();
            InvalidateRect(m_hParent, nullptr, TRUE);
        }
        else if (propertyName == L"TypeDescription") {
            m_typeDescription = m_viewModel->GetTypeDescription();
            InvalidateRect(m_hParent, nullptr, TRUE);
        }
    }

    void StatusPanel::OnPaint(HDC hdc) {
        DrawStatusText(hdc);
        DrawTypeDescription(hdc);
    }

    void StatusPanel::DrawStatusText(HDC hdc) const {
        RECT rc = {
            m_x,
            m_y,
            m_x + m_width,
            m_y + STATUS_H
        };

        HFONT hFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextW(hdc, m_statusText.c_str(), -1, &rc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

    void StatusPanel::DrawTypeDescription(HDC hdc) const {
        const int descY = m_y + STATUS_H + INNER_GAP;

        RECT rc = {
            m_x,
            descY,
            m_x + m_width,
            descY + TYPE_DESC_H
        };

        HBRUSH hBg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        HFONT hFont = CreateFontW(
            14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        const bool         empty = m_typeDescription.empty();
        const std::wstring text = empty ? L"타입을 선택해주세요." : m_typeDescription;

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, empty ? RGB(160, 160, 160) : RGB(30, 30, 30));
        DrawTextW(hdc, text.c_str(), -1, &rc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

}
