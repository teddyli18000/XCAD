#include "pch.h"
#include "../CADDlg.h"

#include <cmath>
#include <vector>

namespace {
const int kMinPolygonPointCount = 3;
const double kPointEqualEpsilon = 1e-9;
const COLORREF kHatchBorderColor = RGB(0, 0, 0);
const int kHatchBorderLineWidth = 1;

//先判断图元是否是闭合形状
bool IsClosedShape(const CLine& shape) {
    const auto& pts = shape.GetPoints();
    if (pts.size() < kMinPolygonPointCount) return false;
    const Point2D& a = pts.front();
    const Point2D& b = pts.back();
    return std::fabs(a.x - b.x) <= kPointEqualEpsilon && std::fabs(a.y - b.y) <= kPointEqualEpsilon;
}
// return: true:图元首尾闭合可用于填充; false:图元不闭合;

//射线法判断点是否在多边形内部
bool PointInPolygon(const std::vector<CPoint>& polygon, const CPoint& p) {
	if (polygon.size() < kMinPolygonPointCount) return false;//点数小于3无法构成多边形

    bool inside = false;
    size_t j = polygon.size() - 1;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const CPoint& pi = polygon[i];
        const CPoint& pj = polygon[j];

        if ((pi.y > p.y) != (pj.y > p.y)) {
            /*
            边： x = pi.x + t * (pj.x - pi.x)，y = pi.y + t * (pj.y - pi.y)
            直线：y=p.y，得到t = (p.y - pi.y) / (pj.y - pi.y)
            代回 x 就得到 intersectX
            */
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
// return: 
// true:内部;
// false:外部;

//按绘制顺序从上到下查找命中点所在的闭合图元
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
// return: std::shared_ptr<CLine>:命中的闭合图元; 
// nullptr:未命中任何闭合图元;
}

//处理填充工具左键点击，提交填充命令
bool CCADDlg::HandleHatchToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && m_bHatchCommandActive)) return false;

    auto shape = FindClosedShapeAtPoint(m_shapeMgr, m_transform, localPt);
    if (!shape) return false;

    m_shapeMgr.ExecuteCommand(std::make_unique<CChangeLineFillCommand>(&m_shapeMgr, shape, true, m_hatchColor));
    m_bHatchPreviewVisible = true;
    m_hatchPreviewPoint = localPt;
    return true;
}
// return: true:已命中闭合图元并执行填充; 
// false:当前状态无效或未命中;

// 处理填充工具鼠标移动，更新填充预览状态
bool CCADDlg::HandleHatchToolMouseMove(const CPoint& localPt, bool inCanvas) {
    if (!m_bHatchCommandActive) return false;

    m_bHatchPreviewVisible = false;
    if (inCanvas) {
        m_hatchPreviewPoint = localPt;
        m_bHatchPreviewVisible = (FindClosedShapeAtPoint(m_shapeMgr, m_transform, localPt) != nullptr);
    }

    return true;
}
// return: true:已处理填充预览更新; 
// false:当前未激活填充命令;

// 绘制填充工具预览效果
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

    if (screenPts.size() < kMinPolygonPointCount) return;

    CBrush hatchBrush(HS_FDIAGONAL, m_hatchColor);
    CBrush* oldBrush = pDC->SelectObject(&hatchBrush);
    CPen borderPen(PS_DOT, kHatchBorderLineWidth, kHatchBorderColor);
    CPen* oldPen = pDC->SelectObject(&borderPen);
    pDC->Polygon(screenPts.data(), static_cast<int>(screenPts.size()));
    pDC->SelectObject(oldPen);
    pDC->SelectObject(oldBrush);
}

