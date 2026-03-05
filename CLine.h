#pragma once

#include "Point2D.h"
#include <afxwin.h>
#include <vector>

class CViewTransform;

// line/polyline model
class CLine {
private:
    std::vector<Point2D> m_points;
    bool m_bSelected;
    COLORREF m_color;
    bool m_hasFill;
    COLORREF m_fillColor;

public:
    // 功能：构造线条对象。
    CLine();
    void AddPoint(const Point2D& pt);//追加顶点
    void SetSelected(bool sel);//设置选中状态
	bool IsSelected() const;//读取选中状态
	void SetColor(COLORREF color);//设置颜色
    COLORREF GetColor() const;//读取颜色
    void SetFill(bool hasFill, COLORREF fillColor);//设置填充属性
    bool HasFill() const;//是否填充
	COLORREF GetFillColor() const;//读取填充颜色

	const std::vector<Point2D>& GetPoints() const;//读取点集

    void Move(double dx, double dy);//平移line
    void Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const;//Draw line
    bool HitTest(const Point2D& pt, double tolerance) const;//鼠标是否命中
};
