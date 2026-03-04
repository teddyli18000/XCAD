#include "pch.h"
#include "CADlgGeometryUtils.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace cad {
namespace dlg {

double NormalizeAngle(double angle) {
    const double twoPi = 6.28318530717958647692;
    while (angle < 0.0) angle += twoPi;
    while (angle >= twoPi) angle -= twoPi;
    return angle;
}

double AngleDistanceCCW(double from, double to) {
    const double f = NormalizeAngle(from);
    const double t = NormalizeAngle(to);
    if (t >= f) return t - f;
    return (6.28318530717958647692 - f) + t;
}

CRect NormalizeRect(const CPoint& a, const CPoint& b) {
    CRect rect(a, b);
    rect.NormalizeRect();
    return rect;
}

bool IsPointInRect(const CPoint& pt, const CRect& rect) {
    return pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom;
}

bool SegmentsIntersect(const CPoint& p1, const CPoint& p2, const CPoint& q1, const CPoint& q2) {
    auto cross = [](const CPoint& a, const CPoint& b, const CPoint& c) {
        return static_cast<double>(b.x - a.x) * static_cast<double>(c.y - a.y)
            - static_cast<double>(b.y - a.y) * static_cast<double>(c.x - a.x);
    };

    auto onSegment = [](const CPoint& a, const CPoint& b, const CPoint& c) {
        return (std::min)(a.x, b.x) <= c.x && c.x <= (std::max)(a.x, b.x)
            && (std::min)(a.y, b.y) <= c.y && c.y <= (std::max)(a.y, b.y);
    };

    const double d1 = cross(p1, p2, q1);
    const double d2 = cross(p1, p2, q2);
    const double d3 = cross(q1, q2, p1);
    const double d4 = cross(q1, q2, p2);

    if (((d1 > 0.0 && d2 < 0.0) || (d1 < 0.0 && d2 > 0.0)) && ((d3 > 0.0 && d4 < 0.0) || (d3 < 0.0 && d4 > 0.0))) {
        return true;
    }

    const double eps = 1e-9;
    if (std::fabs(d1) <= eps && onSegment(p1, p2, q1)) return true;
    if (std::fabs(d2) <= eps && onSegment(p1, p2, q2)) return true;
    if (std::fabs(d3) <= eps && onSegment(q1, q2, p1)) return true;
    if (std::fabs(d4) <= eps && onSegment(q1, q2, p2)) return true;
    return false;
}

bool PolylineIntersectsRect(const CLine& line, const CRect& rect, const CViewTransform& transform) {
    const auto& pts = line.GetPoints();
    if (pts.empty()) return false;

    std::vector<CPoint> screenPts;
    screenPts.reserve(pts.size());
    for (const auto& p : pts) {
        screenPts.push_back(transform.WorldToScreen(p));
    }

    for (const auto& p : screenPts) {
        if (IsPointInRect(p, rect)) return true;
    }

    const CPoint r1(rect.left, rect.top);
    const CPoint r2(rect.right, rect.top);
    const CPoint r3(rect.right, rect.bottom);
    const CPoint r4(rect.left, rect.bottom);

    for (size_t i = 1; i < screenPts.size(); ++i) {
        const CPoint& a = screenPts[i - 1];
        const CPoint& b = screenPts[i];
        if (SegmentsIntersect(a, b, r1, r2) || SegmentsIntersect(a, b, r2, r3) ||
            SegmentsIntersect(a, b, r3, r4) || SegmentsIntersect(a, b, r4, r1)) {
            return true;
        }
    }

    return false;
}

double DistancePointToSegmentSquared(const CPoint& p, const CPoint& a, const CPoint& b) {
    const double dx = static_cast<double>(b.x - a.x);
    const double dy = static_cast<double>(b.y - a.y);
    const double len2 = dx * dx + dy * dy;
    if (len2 < 1e-9) {
        const double px = static_cast<double>(p.x - a.x);
        const double py = static_cast<double>(p.y - a.y);
        return px * px + py * py;
    }

    double t = (static_cast<double>(p.x - a.x) * dx + static_cast<double>(p.y - a.y) * dy) / len2;
    t = (std::max)(0.0, (std::min)(1.0, t));
    const double projX = static_cast<double>(a.x) + t * dx;
    const double projY = static_cast<double>(a.y) + t * dy;
    const double ddx = static_cast<double>(p.x) - projX;
    const double ddy = static_cast<double>(p.y) - projY;
    return ddx * ddx + ddy * ddy;
}

bool PolylineIntersectsCircle(const CLine& line, const CPoint& center, int radius, const CViewTransform& transform) {
    const auto& pts = line.GetPoints();
    if (pts.empty()) return false;

    const double r2 = static_cast<double>(radius) * static_cast<double>(radius);
    std::vector<CPoint> screenPts;
    screenPts.reserve(pts.size());
    for (const auto& p : pts) {
        screenPts.push_back(transform.WorldToScreen(p));
    }

    for (const auto& p : screenPts) {
        const double dx = static_cast<double>(p.x - center.x);
        const double dy = static_cast<double>(p.y - center.y);
        if (dx * dx + dy * dy <= r2) return true;
    }

    for (size_t i = 1; i < screenPts.size(); ++i) {
        if (DistancePointToSegmentSquared(center, screenPts[i - 1], screenPts[i]) <= r2) {
            return true;
        }
    }

    return false;
}

} // namespace dlg
} // namespace cad
