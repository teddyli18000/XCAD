#pragma once

#include "Point2D.h"
#include <afxwin.h>

class CViewTransform {
private:
    double m_scale;
    double m_offsetX;
    double m_offsetY;
    CRect m_screenRect;

public:
    CViewTransform();

    void SetScreenRect(const CRect& rect);
    CRect GetScreenRect() const;

    void Zoom(double factor, const CPoint& screenCenter);
    void Pan(int deltaX, int deltaY);

    CPoint WorldToScreen(const Point2D& pt) const;
    Point2D ScreenToWorld(const CPoint& pt) const;
};
