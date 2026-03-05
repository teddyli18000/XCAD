#include "pch.h"
#include "CLine.h"
#include "CViewTransform.h"

#include <cmath>

CLine::CLine() : m_bSelected(false), m_color(RGB(255, 255, 255)), m_hasFill(false), m_fillColor(RGB(255, 255, 255)) {}

void CLine::AddPoint(const Point2D& pt) { m_points.push_back(pt); }

void CLine::SetSelected(bool sel) { m_bSelected = sel; }

bool CLine::IsSelected() const { return m_bSelected; }

void CLine::SetColor(COLORREF color) { m_color = color; }

COLORREF CLine::GetColor() const { return m_color; }

void CLine::SetFill(bool hasFill, COLORREF fillColor) {
    m_hasFill = hasFill;
    m_fillColor = fillColor;
}

bool CLine::HasFill() const { return m_hasFill; }

COLORREF CLine::GetFillColor() const { return m_fillColor; }

const std::vector<Point2D>& CLine::GetPoints() const { return m_points; }

void CLine::Move(double dx, double dy) {
    for (auto& pt : m_points) {
        pt.x += dx;
        pt.y += dy;
    }
}

void CLine::Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const {
    if (m_points.empty()) return;

    const auto isClosed = [this]() {
        if (m_points.size() < 3) return false;
        const Point2D& a = m_points.front();
        const Point2D& b = m_points.back();
        return std::fabs(a.x - b.x) <= 1e-9 && std::fabs(a.y - b.y) <= 1e-9;
    };

    if (m_hasFill && isClosed()) {
        std::vector<CPoint> screenPts;
        screenPts.reserve(m_points.size());
        for (const auto& p : m_points) {
            screenPts.push_back(transform.WorldToScreen(p));
        }
        if (screenPts.size() >= 3) {
            CBrush fillBrush(m_fillColor);
            CBrush* oldBrush = pDC->SelectObject(&fillBrush);
            CPen* oldPen = static_cast<CPen*>(pDC->SelectStockObject(NULL_PEN));
            pDC->Polygon(screenPts.data(), static_cast<int>(screenPts.size()));
            pDC->SelectObject(oldPen);
            pDC->SelectObject(oldBrush);
        }
    }

    CPen pen(m_bSelected ? PS_DASH : PS_SOLID, 1, m_color);
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
