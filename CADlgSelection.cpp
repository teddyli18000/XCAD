#include "pch.h"
#include "CADDlg.h"
#include "CADlgGeometryUtils.h"

#include <cstddef>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

// 选择与擦除 / selection + eraser
// localPt: 画布局部坐标 / canvas local coordinate

namespace {
const double kPointEpsilon = 1e-9;
const double kLengthEpsilonSquared = 1e-12;
const int kMinClosedPolylineSize = 3;
const int kMinSplitClosedPolylineSize = 4;
const int kSmoothDetectMinPointCount = 10;
const int kSmoothDetectMinDirectionCount = 8;
const double kPi = 3.14159265358979323846;
const double kTwoPi = 6.28318530717958647692;
const double kSmoothTurnThreshold = 0.35;
const double kSmoothRatioThreshold = 0.85;
const int kSelectionClickThreshold = 2;

// 功能：判断两个世界坐标点是否可视为同一点。
bool IsSamePoint(const Point2D& a, const Point2D& b) {
    return std::fabs(a.x - b.x) <= kPointEpsilon && std::fabs(a.y - b.y) <= kPointEpsilon;
}

// 功能：判断折线是否为首尾闭合。
bool IsClosedPolyline(const std::vector<Point2D>& pts) {
    return pts.size() >= kMinClosedPolylineSize && IsSamePoint(pts.front(), pts.back());
}

// 功能：根据点集创建一条基础折线对象。
std::shared_ptr<CLine> CreateLineFromPoints(const std::vector<Point2D>& pts) {
    if (pts.size() < 2) return nullptr;
    auto line = std::make_shared<CLine>();
    for (const auto& p : pts) {
        line->AddPoint(p);
    }
    return line;
}

// 功能：开放折线按命中段切分为左右两段。
std::vector<std::shared_ptr<CLine>> CreateOpenSplitLines(const std::vector<Point2D>& pts, size_t segEndIndex) {
    std::vector<std::shared_ptr<CLine>> result;

    if (segEndIndex > 1) {
        std::vector<Point2D> left(pts.begin(), pts.begin() + static_cast<std::ptrdiff_t>(segEndIndex));
        if (auto line = CreateLineFromPoints(left)) {
            result.push_back(line);
        }
    }

    if (segEndIndex < pts.size() - 1) {
        std::vector<Point2D> right(pts.begin() + static_cast<std::ptrdiff_t>(segEndIndex), pts.end());
        if (auto line = CreateLineFromPoints(right)) {
            result.push_back(line);
        }
    }

    return result;
}

// 功能：闭合折线删除一段后重建保留路径。
std::vector<std::shared_ptr<CLine>> CreateClosedSegmentRemovedLine(const std::vector<Point2D>& pts, size_t segEndIndex) {
    std::vector<std::shared_ptr<CLine>> result;
    if (pts.size() < kMinSplitClosedPolylineSize) return result;

    const size_t vertexCount = pts.size() - 1;
    const size_t removedSeg = segEndIndex - 1;
    std::vector<Point2D> kept;
    kept.reserve(vertexCount);

    const size_t start = (removedSeg + 1) % vertexCount;
    for (size_t step = 0; step < vertexCount; ++step) {
        kept.push_back(pts[(start + step) % vertexCount]);
    }

    if (auto line = CreateLineFromPoints(kept)) {
        result.push_back(line);
    }

    return result;
}

// 功能：估计折线是否更接近平滑曲线。
bool IsSmoothCurvePolyline(const std::vector<Point2D>& pts) {
    if (pts.size() < kSmoothDetectMinPointCount) return false;

    std::vector<double> dirs;
    dirs.reserve(pts.size());
    for (size_t i = 1; i < pts.size(); ++i) {
        const double dx = pts[i].x - pts[i - 1].x;
        const double dy = pts[i].y - pts[i - 1].y;
        if (dx * dx + dy * dy <= kLengthEpsilonSquared) continue;
        dirs.push_back(std::atan2(dy, dx));
    }

    if (dirs.size() < kSmoothDetectMinDirectionCount) return false;

    int smoothTurns = 0;
    int totalTurns = 0;
    for (size_t i = 1; i < dirs.size(); ++i) {
        double d = std::fabs(dirs[i] - dirs[i - 1]);
        while (d > kPi) {
            d = std::fabs(d - kTwoPi);
        }
        ++totalTurns;
        if (d <= kSmoothTurnThreshold) {
            ++smoothTurns;
        }
    }

    if (totalTurns <= 0) return false;
    return (static_cast<double>(smoothTurns) / static_cast<double>(totalTurns)) >= kSmoothRatioThreshold;
}
}

// 功能：清空当前所有图元的选中状态。
void CCADDlg::ClearSelection() {
    for (auto& shape : m_shapeMgr.GetShapes()) {
        shape->SetSelected(false);
    }
}

// 功能：判断当前是否存在已选中的线条。
bool CCADDlg::HasSelectedLines() const {
    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (shape && shape->IsSelected()) {
            return true;
        }
    }
    return false;
}

// 功能：根据框选矩形更新选中图元。
void CCADDlg::ApplySelectionBox() {
    if (m_currentMode != CADMode::MODE_SELECT || m_bEraserCommandActive || m_bDeleteNodeCommandActive || m_bHatchCommandActive) return;

    const CRect box = cad::dlg::NormalizeRect(m_selectBoxStart, m_selectBoxEnd);
    if (box.Width() < kSelectionClickThreshold && box.Height() < kSelectionClickThreshold) {
        ClearSelection();
        return;
    }

    ClearSelection();
    for (auto& shape : m_shapeMgr.GetShapes()) {
        if (cad::dlg::PolylineIntersectsRect(*shape, box, m_transform)) {
            shape->SetSelected(true);
        }
    }
}

// 功能：删除当前已选中的线条。
void CCADDlg::DeleteSelectedLines() {
    std::vector<std::shared_ptr<CLine>> selected;
    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (shape->IsSelected()) {
            selected.push_back(shape);
        }
    }

    if (selected.empty()) return;
    m_shapeMgr.ExecuteCommand(std::make_unique<CDeleteLinesCommand>(&m_shapeMgr, std::move(selected)));
    RefreshCanvas();
}

// 功能：在指定位置执行整线擦除或节点删除。
void CCADDlg::EraseAtPoint(const CPoint& localPt) {
    if (!(m_bEraserCommandActive || m_bDeleteNodeCommandActive)) return;

    if (m_bEraserCommandActive) {
        std::vector<std::shared_ptr<CLine>> hits;
        for (const auto& shape : m_shapeMgr.GetShapes()) {
            if (cad::dlg::PolylineIntersectsCircle(*shape, localPt, m_eraserRadius, m_transform)) {
                hits.push_back(shape);
            }
        }

        if (!hits.empty()) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CDeleteLinesCommand>(&m_shapeMgr, std::move(hits)));
            RefreshCanvas();
        }
        return;
    }

    const double r2 = static_cast<double>(m_eraserRadius) * static_cast<double>(m_eraserRadius);
    std::shared_ptr<CLine> bestShape;
    size_t bestSegEndIndex = 0;
    double bestDist2 = (std::numeric_limits<double>::max)();
    bool bestIsCurve = false;

    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (!cad::dlg::PolylineIntersectsCircle(*shape, localPt, m_eraserRadius, m_transform)) {
            continue;
        }

        const auto& pts = shape->GetPoints();
        if (pts.size() < 2) continue;

        double shapeBestDist2 = (std::numeric_limits<double>::max)();
        size_t shapeBestSegEndIndex = 0;
        for (size_t i = 1; i < pts.size(); ++i) {
            const CPoint a = m_transform.WorldToScreen(pts[i - 1]);
            const CPoint b = m_transform.WorldToScreen(pts[i]);
            const double d2 = cad::dlg::DistancePointToSegmentSquared(localPt, a, b);
            if (d2 < shapeBestDist2) {
                shapeBestDist2 = d2;
                shapeBestSegEndIndex = i;
            }
        }

        if (shapeBestSegEndIndex == 0 || shapeBestDist2 > r2) continue;

        if (shapeBestDist2 < bestDist2) {
            bestDist2 = shapeBestDist2;
            bestShape = shape;
            bestSegEndIndex = shapeBestSegEndIndex;
            bestIsCurve = IsSmoothCurvePolyline(pts);
        }
    }

    if (!bestShape) return;

    if (bestIsCurve) {
        std::vector<std::shared_ptr<CLine>> hits;
        hits.push_back(bestShape);
        m_shapeMgr.ExecuteCommand(std::make_unique<CDeleteLinesCommand>(&m_shapeMgr, std::move(hits)));
        RefreshCanvas();
        return;
    }

    const auto& bestPts = bestShape->GetPoints();
    std::vector<std::shared_ptr<CLine>> replacements;
    if (IsClosedPolyline(bestPts)) {
        replacements = CreateClosedSegmentRemovedLine(bestPts, bestSegEndIndex);
    } else {
        replacements = CreateOpenSplitLines(bestPts, bestSegEndIndex);
    }

    m_shapeMgr.ExecuteCommand(std::make_unique<CReplaceLineCommand>(&m_shapeMgr, bestShape, std::move(replacements)));
    RefreshCanvas();
}
