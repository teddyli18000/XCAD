#include "pch.h"
#include "../CADDlg.h"

#include <cmath>
#include <vector>

namespace {
bool IsClosedShape(const CLine& shape) {
    const auto& pts = shape.GetPoints();
    if (pts.size() < 3) return false;
    const Point2D& a = pts.front();
    const Point2D& b = pts.back();
    return std::fabs(a.x - b.x) <= 1e-9 && std::fabs(a.y - b.y) <= 1e-9;
}

bool PointInPolygon(const std::vector<CPoint>& polygon, const CPoint& p) {
    if (polygon.size() < 3) return false;

    bool inside = false;
    size_t j = polygon.size() - 1;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const CPoint& pi = polygon[i];
        const CPoint& pj = polygon[j];

        if ((pi.y > p.y) != (pj.y > p.y)) {
            const double intersectX = static_cast<double>(pi.x)
                + static_cast<double>(pj.x - pi.x) * static_cast<double>(p.y - pi.y)
                / static_cast<double>(pj.y - pi.y);
            if (static_cast<double>(p.x) < intersectX) {
                inside = !inside;
            }
        }

        j = i;
    }

    return inside;
}

std::shared_ptr<CLine> FindClosedShapeAtPoint(const CShapeManager& shapeMgr, const CViewTransform& transform, const CPoint& localPt) {
    const auto& shapes = shapeMgr.GetShapes();
    for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
        const auto& shape = *it;
        if (!shape || !IsClosedShape(*shape)) continue;

        const auto& pts = shape->GetPoints();
        std::vector<CPoint> screenPts;
        screenPts.reserve(pts.size());
        for (const auto& pt : pts) {
            screenPts.push_back(transform.WorldToScreen(pt));
        }

        if (PointInPolygon(screenPts, localPt)) {
            return shape;
        }
    }

    return nullptr;
}
}

bool CCADDlg::HandleHatchToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && m_bHatchCommandActive)) return false;

    auto shape = FindClosedShapeAtPoint(m_shapeMgr, m_transform, localPt);
    if (!shape) return false;

    m_shapeMgr.ExecuteCommand(std::make_unique<CChangeLineFillCommand>(&m_shapeMgr, shape, true, m_hatchColor));
    m_bHatchPreviewVisible = true;
    m_hatchPreviewPoint = localPt;
    return true;
}

bool CCADDlg::HandleHatchToolMouseMove(const CPoint& localPt, bool inCanvas) {
    if (!m_bHatchCommandActive) return false;

    m_bHatchPreviewVisible = false;
    if (inCanvas) {
        m_hatchPreviewPoint = localPt;
        m_bHatchPreviewVisible = (FindClosedShapeAtPoint(m_shapeMgr, m_transform, localPt) != nullptr);
    }

    return true;
}

void CCADDlg::DrawHatchPreview(CDC* pDC) {
    if (!pDC || !m_bHatchCommandActive || !m_bHatchPreviewVisible) return;

    auto shape = FindClosedShapeAtPoint(m_shapeMgr, m_transform, m_hatchPreviewPoint);
    if (!shape) return;

    const auto& pts = shape->GetPoints();
    std::vector<CPoint> screenPts;
    screenPts.reserve(pts.size());
    for (const auto& pt : pts) {
        screenPts.push_back(m_transform.WorldToScreen(pt));
    }

    if (screenPts.size() < 3) return;

    CBrush hatchBrush(HS_FDIAGONAL, m_hatchColor);
    CBrush* oldBrush = pDC->SelectObject(&hatchBrush);
    CPen borderPen(PS_DOT, 1, RGB(0, 0, 0));
    CPen* oldPen = pDC->SelectObject(&borderPen);
    pDC->Polygon(screenPts.data(), static_cast<int>(screenPts.size()));
    pDC->SelectObject(oldPen);
    pDC->SelectObject(oldBrush);
}
