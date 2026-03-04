#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"
#include "CADlgGeometryUtils.h"

#include <cmath>
#include <memory>

// 渲染模块 / rendering module

void CCADDlg::DrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
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

    m_shapeMgr.DrawAll(&memDC, m_transform, m_bShowPoints);

    if (m_bIsDrawing && m_pCurrentLine) {
        m_pCurrentLine->Draw(&memDC, m_transform, true);
    }

    if (m_bCircleCommandActive && m_bCircleCenterPicked) {
        const double dx = m_circlePreviewPoint.x - m_circleCenter.x;
        const double dy = m_circlePreviewPoint.y - m_circleCenter.y;
        const double radius = std::sqrt(dx * dx + dy * dy);

        CPoint centerPt = m_transform.WorldToScreen(m_circleCenter);
        CPoint previewPt = m_transform.WorldToScreen(m_circlePreviewPoint);

        CPen dashPen(PS_DASH, 1, RGB(200, 200, 200));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);

        memDC.MoveTo(centerPt);
        memDC.LineTo(previewPt);

        CPoint radiusPt = m_transform.WorldToScreen(Point2D(m_circleCenter.x + radius, m_circleCenter.y));
        int r = radiusPt.x - centerPt.x;
        if (r < 0) r = -r;
        if (r > 0) {
            memDC.Ellipse(centerPt.x - r, centerPt.y - r, centerPt.x + r, centerPt.y + r);
        }

        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_bRectangleCommandActive && m_bRectangleFirstPicked) {
        CPoint p1 = m_transform.WorldToScreen(m_rectFirstPoint);
        CPoint p3 = m_transform.WorldToScreen(m_rectPreviewPoint);
        CPoint p2(p3.x, p1.y);
        CPoint p4(p1.x, p3.y);

        CPen dashPen(PS_DASH, 1, RGB(200, 200, 200));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);

        memDC.MoveTo(p1);
        memDC.LineTo(p2);
        memDC.LineTo(p3);
        memDC.LineTo(p4);
        memDC.LineTo(p1);

        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_bArcCommandActive && m_arcPointCount > 0) {
        CPen dashPen(PS_DASH, 1, RGB(200, 200, 200));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);

        if (m_arcPointCount == 1) {
            CPoint ps = m_transform.WorldToScreen(m_arcStartPoint);
            CPoint pp = m_transform.WorldToScreen(m_arcPreviewPoint);
            memDC.MoveTo(ps);
            memDC.LineTo(pp);
        } else if (m_arcPointCount == 2) {
            std::shared_ptr<CLine> previewArc = CreateArcPolylineByThreePoints(m_arcStartPoint, m_arcSecondPoint, m_arcPreviewPoint, 72);
            const auto& pts = previewArc->GetPoints();
            if (!pts.empty()) {
                CPoint startPt = m_transform.WorldToScreen(pts[0]);
                memDC.MoveTo(startPt);
                for (size_t i = 1; i < pts.size(); ++i) {
                    memDC.LineTo(m_transform.WorldToScreen(pts[i]));
                }
            }
        }

        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox) {
        CRect box = cad::dlg::NormalizeRect(m_selectBoxStart, m_selectBoxEnd);
        CPen dashPen(PS_DASH, 1, RGB(255, 255, 255));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);
        memDC.Rectangle(&box);
        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_bEraserCommandActive && m_bEraserCursorVisible) {
        CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
        CPen* oldPen = memDC.SelectObject(&pen);
        CBrush* oldBrush = static_cast<CBrush*>(memDC.SelectStockObject(NULL_BRUSH));
        memDC.Ellipse(m_eraserCursor.x - m_eraserRadius, m_eraserCursor.y - m_eraserRadius,
            m_eraserCursor.x + m_eraserRadius, m_eraserCursor.y + m_eraserRadius);
        memDC.SelectObject(oldBrush);
        memDC.SelectObject(oldPen);
    }

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
