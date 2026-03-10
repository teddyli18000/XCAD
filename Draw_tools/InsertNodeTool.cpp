#include "pch.h"
#include "../CADDlg.h"
#include "../CADlgGeometryUtils.h"

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace {
const double kPointEpsilon = 1e-9;

bool IsSamePoint(const Point2D& a, const Point2D& b) {
    return std::fabs(a.x - b.x) <= kPointEpsilon && std::fabs(a.y - b.y) <= kPointEpsilon;
}
// return: 
// true:两点相同; 
// false:两点不同;

Point2D ProjectPointToSegment(const CPoint& p, const CPoint& a, const CPoint& b) {
    const double dx = static_cast<double>(b.x - a.x);
    const double dy = static_cast<double>(b.y - a.y);
    const double len2 = dx * dx + dy * dy;
    if (len2 <= kPointEpsilon) {
        return Point2D(static_cast<double>(a.x), static_cast<double>(a.y));
    }

    double t = (static_cast<double>(p.x - a.x) * dx + static_cast<double>(p.y - a.y) * dy) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    return Point2D(static_cast<double>(a.x) + t * dx, static_cast<double>(a.y) + t * dy);
}
// return: Point2D:点在目标线段上的投影点（screen坐标系数值）;

std::shared_ptr<CLine> CreateInsertedLine(const CLine& original, size_t segEndIndex, const Point2D& insertedPoint) {
    const auto& pts = original.GetPoints();
    if (pts.size() < 2 || segEndIndex == 0 || segEndIndex >= pts.size()) return nullptr;

    auto replaced = std::make_shared<CLine>();
    for (size_t i = 0; i < pts.size(); ++i) {
        if (i == segEndIndex) {
            replaced->AddPoint(insertedPoint);
        }
        replaced->AddPoint(pts[i]);
    }

    replaced->SetSelected(original.IsSelected());
    replaced->SetColor(original.GetColor());
    replaced->SetFill(original.HasFill(), original.GetFillColor());
    replaced->SetTextEntity(original.IsTextEntity());
    replaced->SetTextContent(original.GetTextContent());
    replaced->SetEntityType(original.GetEntityType());
    replaced->SetEntityData(original.GetEntityData());
    return replaced;
}
// return: 
// std::shared_ptr<CLine>:插入节点后的新折线; 
// nullptr:输入参数无效无法插入;
}

//插入节点模式下，左键点击时按最近线段插入新节点
bool CCADDlg::HandleInsertNodeToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && m_bInsertNodeCommandActive)) return false;

    m_eraserCursor = localPt;
    m_bEraserCursorVisible = true;
    InsertNodeAtPoint(localPt);
    return true;
}
// return: true:已处理插入节点点击;
// false:当前未激活插入节点命令;

//更新插入节点模式的容差光标位置
bool CCADDlg::HandleInsertNodeToolMouseMove(const CPoint& localPt, bool inCanvas) {
    if (!m_bInsertNodeCommandActive) return false;

    m_bEraserCursorVisible = inCanvas;
    if (inCanvas) {
        m_eraserCursor = localPt;
    }
    return true;
}
// return: true:已更新插入节点容差光标; 
// false:当前未激活插入节点命令;

//在容差范围内查找最近线段，并插入一个新节点
void CCADDlg::InsertNodeAtPoint(const CPoint& localPt) {
    if (!m_bInsertNodeCommandActive) return;

    const double r2 = static_cast<double>(m_eraserRadius) * static_cast<double>(m_eraserRadius);
    std::shared_ptr<CLine> bestShape;
    size_t bestSegEndIndex = 0;
    double bestDist2 = (std::numeric_limits<double>::max)();
    Point2D bestProjectedScreen(0.0, 0.0);

    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (!shape || shape->IsTextEntity()) continue;
        if (!cad::dlg::PolylineIntersectsCircle(*shape, localPt, m_eraserRadius, m_transform)) continue;

        const auto& pts = shape->GetPoints();
        if (pts.size() < 2) continue;

        for (size_t i = 1; i < pts.size(); ++i) {
            const CPoint a = m_transform.WorldToScreen(pts[i - 1]);
            const CPoint b = m_transform.WorldToScreen(pts[i]);
            const double d2 = cad::dlg::DistancePointToSegmentSquared(localPt, a, b);
            if (d2 < bestDist2) {
                bestDist2 = d2;
                bestShape = shape;
                bestSegEndIndex = i;
                bestProjectedScreen = ProjectPointToSegment(localPt, a, b);
            }
        }
    }

    if (!bestShape || bestSegEndIndex == 0 || bestDist2 > r2) return;

    const CPoint projectedScreenPt(static_cast<int>(std::lround(bestProjectedScreen.x)), static_cast<int>(std::lround(bestProjectedScreen.y)));
    const Point2D insertedWorld = m_transform.ScreenToWorld(projectedScreenPt);

    const auto& srcPts = bestShape->GetPoints();
    if (IsSamePoint(insertedWorld, srcPts[bestSegEndIndex - 1]) || IsSamePoint(insertedWorld, srcPts[bestSegEndIndex])) {
        return;
    }

    auto insertedLine = CreateInsertedLine(*bestShape, bestSegEndIndex, insertedWorld);
    if (!insertedLine) return;

    std::vector<std::shared_ptr<CLine>> replacements;
    replacements.push_back(insertedLine);
    m_shapeMgr.ExecuteCommand(std::make_unique<CReplaceLineCommand>(&m_shapeMgr, bestShape, std::move(replacements)));
    RefreshCanvas();
}
