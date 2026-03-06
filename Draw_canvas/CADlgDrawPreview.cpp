#include "pch.h"
#include "../CADDlg.h"

#include <cmath>
#include <memory>

namespace {
const COLORREF kPreviewDashColor = RGB(200, 200, 200);
const int kPreviewLineWidth = 1;
const int kArcPreviewSegments = 72;
}

//绘制当前命令的动态预览图形
void CCADDlg::DrawPreview(CDC* pDC) {
    if (!pDC) return;

    if (m_bIsDrawing && m_pCurrentLine) {
        m_pCurrentLine->Draw(pDC, m_transform, true);
    }

    if (m_bCircleCommandActive && m_bCircleCenterPicked) {
        const double dx = m_circlePreviewPoint.x - m_circleCenter.x;
        const double dy = m_circlePreviewPoint.y - m_circleCenter.y;
        const double radius = std::sqrt(dx * dx + dy * dy);

        CPoint centerPt = m_transform.WorldToScreen(m_circleCenter);
        CPoint previewPt = m_transform.WorldToScreen(m_circlePreviewPoint);

        CPen dashPen(PS_DASH, kPreviewLineWidth, kPreviewDashColor);
        CPen* oldPen = pDC->SelectObject(&dashPen);
        CBrush* oldBrush = static_cast<CBrush*>(pDC->SelectStockObject(NULL_BRUSH));
        int oldBkMode = pDC->SetBkMode(TRANSPARENT);

        pDC->MoveTo(centerPt);
        pDC->LineTo(previewPt);

        CPoint radiusPt = m_transform.WorldToScreen(Point2D(m_circleCenter.x + radius, m_circleCenter.y));
        int r = radiusPt.x - centerPt.x;
        if (r < 0) r = -r;
        if (r > 0) {
            pDC->Ellipse(centerPt.x - r, centerPt.y - r, centerPt.x + r, centerPt.y + r);
        }

        pDC->SetBkMode(oldBkMode);
        pDC->SelectObject(oldBrush);
        pDC->SelectObject(oldPen);
    }

    if (m_bRectangleCommandActive && m_bRectangleFirstPicked) {
        CPoint p1 = m_transform.WorldToScreen(m_rectFirstPoint);
        CPoint p3 = m_transform.WorldToScreen(m_rectPreviewPoint);
        CPoint p2(p3.x, p1.y);
        CPoint p4(p1.x, p3.y);

        CPen dashPen(PS_DASH, kPreviewLineWidth, kPreviewDashColor);
        CPen* oldPen = pDC->SelectObject(&dashPen);
        int oldBkMode = pDC->SetBkMode(TRANSPARENT);

        pDC->MoveTo(p1);
        pDC->LineTo(p2);
        pDC->LineTo(p3);
        pDC->LineTo(p4);
        pDC->LineTo(p1);

        pDC->SetBkMode(oldBkMode);
        pDC->SelectObject(oldPen);
    }

    if (m_bTextCommandActive && m_bTextFirstPicked) {
        CPoint p1 = m_transform.WorldToScreen(m_textFirstPoint);
        CPoint p3 = m_transform.WorldToScreen(m_textPreviewPoint);
        CRect textRect(min(p1.x, p3.x), min(p1.y, p3.y), max(p1.x, p3.x), max(p1.y, p3.y));

        CPen dashPen(PS_DASH, kPreviewLineWidth, kPreviewDashColor);
        CPen* oldPen = pDC->SelectObject(&dashPen);
        CBrush* oldBrush = static_cast<CBrush*>(pDC->SelectStockObject(NULL_BRUSH));
        int oldBkMode = pDC->SetBkMode(TRANSPARENT);

        pDC->Rectangle(&textRect);

        pDC->SetBkMode(oldBkMode);
        pDC->SelectObject(oldBrush);
        pDC->SelectObject(oldPen);
    }

    if (m_bArcCommandActive && m_arcPointCount > 0) {
        CPen dashPen(PS_DASH, kPreviewLineWidth, kPreviewDashColor);
        CPen* oldPen = pDC->SelectObject(&dashPen);
        int oldBkMode = pDC->SetBkMode(TRANSPARENT);

        if (m_arcPointCount == 1) {
            CPoint ps = m_transform.WorldToScreen(m_arcStartPoint);
            CPoint pp = m_transform.WorldToScreen(m_arcPreviewPoint);
            pDC->MoveTo(ps);
            pDC->LineTo(pp);
        } else if (m_arcPointCount == 2) {
            std::shared_ptr<CLine> previewArc = CreateArcPolylineByThreePoints(m_arcStartPoint, m_arcSecondPoint, m_arcPreviewPoint, kArcPreviewSegments);
            const auto& pts = previewArc->GetPoints();
            if (!pts.empty()) {
                CPoint startPt = m_transform.WorldToScreen(pts[0]);
                pDC->MoveTo(startPt);
                for (size_t i = 1; i < pts.size(); ++i) {
                    pDC->LineTo(m_transform.WorldToScreen(pts[i]));
                }
            }
        }

        pDC->SetBkMode(oldBkMode);
        pDC->SelectObject(oldPen);
    }

    DrawHatchPreview(pDC);
}
