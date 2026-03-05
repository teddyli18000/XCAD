#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

// 渲染模块 / rendering module

namespace {
bool TryGetPaletteColor(int ctrlId, COLORREF& color) {
    switch (ctrlId) {
    case IDC_COLOR_WHITE: color = RGB(255, 255, 255); return true;
    case IDC_COLOR_RED: color = RGB(255, 0, 0); return true;
    case IDC_COLOR_YELLOW: color = RGB(255, 255, 0); return true;
    case IDC_COLOR_GREEN: color = RGB(0, 255, 0); return true;
    case IDC_COLOR_CYAN: color = RGB(0, 255, 255); return true;
    case IDC_COLOR_BLUE: color = RGB(0, 0, 255); return true;
    case IDC_COLOR_MAGENTA: color = RGB(255, 0, 255); return true;
    default: return false;
    }
}
}

void CCADDlg::DrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    COLORREF paletteColor = RGB(0, 0, 0);
    if (TryGetPaletteColor(nIDCtl, paletteColor)) {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);

        CRect rc = lpDrawItemStruct->rcItem;
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));

        dc.Draw3dRect(&rc, RGB(30, 30, 30), RGB(220, 220, 220));

        CRect square = rc;
        square.DeflateRect(2, 2);
        CBrush brush(paletteColor);
        CPen pen(PS_SOLID, 1, RGB(20, 20, 20));
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.Rectangle(&square);
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);

        if ((lpDrawItemStruct->itemState & ODS_FOCUS) != 0) {
            dc.DrawFocusRect(&rc);
        }

        dc.Detach();
        return;
    }

    if (nIDCtl == IDC_ABOUT_ICON) {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);

        CRect rc = lpDrawItemStruct->rcItem;
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));

        CRect circleRc = rc;
        circleRc.DeflateRect(1, 1);

        CPen pen(PS_SOLID, 1, RGB(0, 85, 170));
        CBrush brush(RGB(0, 122, 204));
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.Ellipse(&circleRc);
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);

        int oldBkMode = dc.SetBkMode(TRANSPARENT);
        COLORREF oldTextColor = dc.SetTextColor(RGB(255, 255, 255));

        CFont font;
        font.CreateFont(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
        CFont* oldFont = dc.SelectObject(&font);

        dc.DrawText(_T("?"), &circleRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        dc.SelectObject(oldFont);
        dc.SetTextColor(oldTextColor);
        dc.SetBkMode(oldBkMode);

        if ((lpDrawItemStruct->itemState & ODS_FOCUS) != 0) {
            dc.DrawFocusRect(&rc);
        }

        dc.Detach();
        return;
    }

    UNREFERENCED_PARAMETER(lpDrawItemStruct);
}

void CCADDlg::OnPaint() {
    CPaintDC dc(this);

    CRect rect = m_transform.GetScreenRect();
    if (rect.IsRectEmpty()) return;

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBitmap);

    memDC.FillSolidRect(0, 0, rect.Width(), rect.Height(), RGB(33, 33, 33));

    DrawModel(&memDC);
    DrawPreview(&memDC);
    DrawSelection(&memDC);
    DrawCursor(&memDC);

    dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void CCADDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);

    if (GetDlgItem(IDC_DRAW_AREA)) {
        CRect rect;
        GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&rect);
        ScreenToClient(&rect);
        m_transform.SetScreenRect(rect);
        RefreshCanvas();
    }
}
