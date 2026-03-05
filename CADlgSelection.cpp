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
bool IsSamePoint(const Point2D& a, const Point2D& b) {
    return std::fabs(a.x - b.x) <= 1e-9 && std::fabs(a.y - b.y) <= 1e-9;
}

bool IsClosedPolyline(const std::vector<Point2D>& pts) {
    return pts.size() >= 3 && IsSamePoint(pts.front(), pts.back());
}

std::shared_ptr<CLine> CreateLineFromPoints(const std::vector<Point2D>& pts) {
    if (pts.size() < 2) return nullptr;
    auto line = std::make_shared<CLine>();
    for (const auto& p : pts) {
        line->AddPoint(p);
    }
    return line;
}

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

std::vector<std::shared_ptr<CLine>> CreateClosedSegmentRemovedLine(const std::vector<Point2D>& pts, size_t segEndIndex) {
    std::vector<std::shared_ptr<CLine>> result;
    if (pts.size() < 4) return result;

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

bool IsSmoothCurvePolyline(const std::vector<Point2D>& pts) {
    if (pts.size() < 10) return false;

    std::vector<double> dirs;
    dirs.reserve(pts.size());
    for (size_t i = 1; i < pts.size(); ++i) {
        const double dx = pts[i].x - pts[i - 1].x;
        const double dy = pts[i].y - pts[i - 1].y;
        if (dx * dx + dy * dy <= 1e-12) continue;
        dirs.push_back(std::atan2(dy, dx));
    }

    if (dirs.size() < 8) return false;

    int smoothTurns = 0;
    int totalTurns = 0;
    for (size_t i = 1; i < dirs.size(); ++i) {
        double d = std::fabs(dirs[i] - dirs[i - 1]);
        while (d > 3.14159265358979323846) {
            d = std::fabs(d - 6.28318530717958647692);
        }
        ++totalTurns;
        if (d <= 0.35) {
            ++smoothTurns;
        }
    }

    if (totalTurns <= 0) return false;
    return (static_cast<double>(smoothTurns) / static_cast<double>(totalTurns)) >= 0.85;
}
}

void CCADDlg::ClearSelection() {
    for (auto& shape : m_shapeMgr.GetShapes()) {
        shape->SetSelected(false);
    }
}

void CCADDlg::ApplySelectionBox() {
    if (m_currentMode != CADMode::MODE_SELECT || m_bEraserCommandActive || m_bDeleteNodeCommandActive || m_bHatchCommandActive) return;

    const CRect box = cad::dlg::NormalizeRect(m_selectBoxStart, m_selectBoxEnd);
    if (box.Width() < 2 && box.Height() < 2) {
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
