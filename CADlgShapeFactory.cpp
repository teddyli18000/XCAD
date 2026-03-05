#include "pch.h"
#include "CADDlg.h"
#include "CADlgGeometryUtils.h"

#include <cmath>
#include <memory>

// 图元构造 / shape factory
// 参数说明 / params:
// center:first/second/start/through/end 均为 world 坐标
// segments 为离散段数 / tessellation count

namespace {
const int kMinCircleSegments = 8;
const int kArcPreviewMinSegments = 8;
const double kPi = 3.14159265358979323846;
const double kTwoPi = 6.28318530717958647692;
const double kGeomEpsilon = 1e-9;
}

// 功能：根据圆心和半径构造离散圆折线。
std::shared_ptr<CLine> CCADDlg::CreateCirclePolyline(const Point2D& center, double radius, int segments) const {
    std::shared_ptr<CLine> circle = std::make_shared<CLine>();
    if (segments < kMinCircleSegments) segments = kMinCircleSegments;
    if (radius <= 0.0) return circle;

    EntityData data;
    data.Center = center;
    data.Radius = radius;
    data.StartAngle = 0.0;
    data.EndAngle = kTwoPi;
    circle->SetEntityType(EntityType::CIRCLE);
    circle->SetEntityData(data);

    for (int i = 0; i <= segments; ++i) {
        const double angle = (kTwoPi * i) / static_cast<double>(segments);
        circle->AddPoint(Point2D(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)));
    }

    return circle;
}

// 功能：根据对角点构造闭合矩形折线。
std::shared_ptr<CLine> CCADDlg::CreateRectanglePolyline(const Point2D& first, const Point2D& second) const {
    std::shared_ptr<CLine> rect = std::make_shared<CLine>();
    rect->SetEntityType(EntityType::RECTANGLE);
    const Point2D p1(first.x, first.y);
    const Point2D p2(second.x, first.y);
    const Point2D p3(second.x, second.y);
    const Point2D p4(first.x, second.y);

    rect->AddPoint(p1);
    rect->AddPoint(p2);
    rect->AddPoint(p3);
    rect->AddPoint(p4);
    rect->AddPoint(p1);
    return rect;
}

// 功能：由三点构造圆弧折线（起点-过点-终点）。
std::shared_ptr<CLine> CCADDlg::CreateArcPolylineByThreePoints(const Point2D& start, const Point2D& through, const Point2D& end, int segments) const {
    std::shared_ptr<CLine> arc = std::make_shared<CLine>();

    const double d = 2.0 * (start.x * (through.y - end.y) + through.x * (end.y - start.y) + end.x * (start.y - through.y));
    if (std::fabs(d) < kGeomEpsilon) {
        arc->AddPoint(start);
        arc->AddPoint(through);
        arc->AddPoint(end);
        return arc;
    }

    const double s2 = start.x * start.x + start.y * start.y;
    const double t2 = through.x * through.x + through.y * through.y;
    const double e2 = end.x * end.x + end.y * end.y;

    const double cx = (s2 * (through.y - end.y) + t2 * (end.y - start.y) + e2 * (start.y - through.y)) / d;
    const double cy = (s2 * (end.x - through.x) + t2 * (start.x - end.x) + e2 * (through.x - start.x)) / d;
    const Point2D center(cx, cy);

    const double rdx = start.x - center.x;
    const double rdy = start.y - center.y;
    const double radius = std::sqrt(rdx * rdx + rdy * rdy);
    if (radius < kGeomEpsilon) {
        arc->AddPoint(start);
        arc->AddPoint(end);
        return arc;
    }

    const double aStart = std::atan2(start.y - center.y, start.x - center.x);
    const double aThrough = std::atan2(through.y - center.y, through.x - center.x);
    const double aEnd = std::atan2(end.y - center.y, end.x - center.x);

    const double spanCCW = cad::dlg::AngleDistanceCCW(aStart, aEnd);
    const double throughCCW = cad::dlg::AngleDistanceCCW(aStart, aThrough);
    const bool ccw = throughCCW <= spanCCW;

    EntityData data;
    data.Center = center;
    data.Radius = radius;
    data.StartAngle = aStart;
    if (ccw) {
        data.EndAngle = aStart + spanCCW;
    } else {
        const double spanCW = kTwoPi - spanCCW;
        data.EndAngle = aStart - spanCW;
    }
    arc->SetEntityType(EntityType::ARC);
    arc->SetEntityData(data);

    if (segments < kArcPreviewMinSegments) segments = kArcPreviewMinSegments;
    for (int i = 0; i <= segments; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(segments);
        double angle = 0.0;
        if (ccw) {
            angle = aStart + spanCCW * t;
        } else {
            const double spanCW = kTwoPi - spanCCW;
            angle = aStart - spanCW * t;
        }
        arc->AddPoint(Point2D(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)));
    }

    return arc;
}
