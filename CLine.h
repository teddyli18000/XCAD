#pragma once

#include "Point2D.h"
#include <afxwin.h>
#include <vector>

class CViewTransform;

class CLine {
private:
    std::vector<Point2D> m_points;
    bool m_bSelected;
    COLORREF m_color;
    bool m_hasFill;
    COLORREF m_fillColor;

public:
    CLine();

    void AddPoint(const Point2D& pt);
    void SetSelected(bool sel);
    bool IsSelected() const;
    void SetColor(COLORREF color);
    COLORREF GetColor() const;
    void SetFill(bool hasFill, COLORREF fillColor);
    bool HasFill() const;
    COLORREF GetFillColor() const;

    const std::vector<Point2D>& GetPoints() const;

    void Move(double dx, double dy);
    void Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const;
    bool HitTest(const Point2D& pt, double tolerance) const;
};
