#include "pch.h"
#include "CViewTransform.h"

CViewTransform::CViewTransform() : m_scale(1.0), m_offsetX(0.0), m_offsetY(0.0) {}

void CViewTransform::SetScreenRect(const CRect& rect) { m_screenRect = rect; }

CRect CViewTransform::GetScreenRect() const { return m_screenRect; }

void CViewTransform::Zoom(double factor, const CPoint& screenCenter) {
    if (factor <= 0.0) return;

    Point2D worldCenter = ScreenToWorld(screenCenter);
    m_scale *= factor;
    if (m_scale < 1e-9) m_scale = 1e-9;
    Point2D newWorldCenter = ScreenToWorld(screenCenter);
    m_offsetX += (newWorldCenter.x - worldCenter.x);
    m_offsetY += (newWorldCenter.y - worldCenter.y);
}

void CViewTransform::Pan(int deltaX, int deltaY) {
    m_offsetX += deltaX / m_scale;
    m_offsetY -= deltaY / m_scale;
}

CPoint CViewTransform::WorldToScreen(const Point2D& pt) const {
    int sx = static_cast<int>((pt.x + m_offsetX) * m_scale);
    int sy = m_screenRect.Height() - static_cast<int>((pt.y + m_offsetY) * m_scale);
    return CPoint(sx, sy);
}

Point2D CViewTransform::ScreenToWorld(const CPoint& pt) const {
    double wx = pt.x / m_scale - m_offsetX;
    double wy = (m_screenRect.Height() - pt.y) / m_scale - m_offsetY;
    return Point2D(wx, wy);
}
