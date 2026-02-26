#include "pch.h"
#include "CLine.h"
#include "CViewTransform.h"

CLine::CLine() : m_bSelected(false) {}

void CLine::AddPoint(const Point2D& pt) { m_points.push_back(pt); }

void CLine::SetSelected(bool sel) { m_bSelected = sel; }

bool CLine::IsSelected() const { return m_bSelected; }

const std::vector<Point2D>& CLine::GetPoints() const { return m_points; }

void CLine::Move(double dx, double dy) {
    for (auto& pt : m_points) {
        pt.x += dx;
        pt.y += dy;
    }
}

void CLine::Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const {
    if (m_points.empty()) return;

    CPen pen(m_bSelected ? PS_DASH : PS_SOLID, 1, m_bSelected ? RGB(255, 0, 0) : RGB(255, 255, 255));
    CPen* pOldPen = pDC->SelectObject(&pen);

    CPoint startPt = transform.WorldToScreen(m_points[0]);
    pDC->MoveTo(startPt);

    for (size_t i = 1; i < m_points.size(); ++i) {
        pDC->LineTo(transform.WorldToScreen(m_points[i]));
    }

    pDC->SelectObject(pOldPen);

    if (bShowPoints || m_bSelected) {
        CBrush brush(RGB(0, 0, 255));
        CBrush* pOldBrush = pDC->SelectObject(&brush);
        for (const auto& pt : m_points) {
            CPoint spt = transform.WorldToScreen(pt);
            pDC->Rectangle(spt.x - 3, spt.y - 3, spt.x + 3, spt.y + 3);
        }
        pDC->SelectObject(pOldBrush);
    }
}

bool CLine::HitTest(const Point2D& pt, double tolerance) const {
    UNREFERENCED_PARAMETER(pt);
    UNREFERENCED_PARAMETER(tolerance);
    return false;
}
