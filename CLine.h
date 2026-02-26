#pragma once

#include "Point2D.h"
#include <afxwin.h>
#include <vector>

class CViewTransform;

class CLine {
private:
    std::vector<Point2D> m_points;
    bool m_bSelected;

public:
    CLine();

    void AddPoint(const Point2D& pt);
    void SetSelected(bool sel);
    bool IsSelected() const;

    const std::vector<Point2D>& GetPoints() const;

    void Move(double dx, double dy);
    void Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const;
    bool HitTest(const Point2D& pt, double tolerance) const;
};
