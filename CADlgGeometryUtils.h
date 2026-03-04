#pragma once

#include "CLine.h"
#include "CViewTransform.h"

namespace cad {
namespace dlg {

// 角度标准化 / normalize angle to [0, 2pi)
double NormalizeAngle(double angle);

// 逆时针角距 / ccw angle distance
double AngleDistanceCCW(double from, double to);

// 归一化矩形 / normalize drag rect
CRect NormalizeRect(const CPoint& a, const CPoint& b);

// 点在矩形内 / point in rect
bool IsPointInRect(const CPoint& pt, const CRect& rect);

// 线段相交 / segment intersection
bool SegmentsIntersect(const CPoint& p1, const CPoint& p2, const CPoint& q1, const CPoint& q2);

// 折线与矩形相交 / polyline vs rect
bool PolylineIntersectsRect(const CLine& line, const CRect& rect, const CViewTransform& transform);

// 点到线段距离平方 / squared distance point to segment
double DistancePointToSegmentSquared(const CPoint& p, const CPoint& a, const CPoint& b);

// 折线与圆相交 / polyline vs circle
bool PolylineIntersectsCircle(const CLine& line, const CPoint& center, int radius, const CViewTransform& transform);

} // namespace dlg
} // namespace cad
