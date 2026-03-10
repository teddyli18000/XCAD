#pragma once

#include "Point2D.h"
#include <afxwin.h>
#include <string>
#include <vector>

class CViewTransform;

enum class EntityType {
    LINE,
    CIRCLE,
    ARC,
    RECTANGLE,
    TRIANGLE
};

struct EntityData {
    Point2D Center;
    double Radius;
    double StartAngle;
    double EndAngle;

    EntityData()
        : Center(), Radius(0.0), StartAngle(0.0), EndAngle(0.0) {
    }
};

// line/polyline model
class CLine {
private:
    std::vector<Point2D> m_points;
    bool m_bSelected;
    COLORREF m_color;
    bool m_hasFill;
    COLORREF m_fillColor;
    bool m_isTextEntity;
    std::wstring m_textContent;
    EntityType m_entityType;
    EntityData m_entityData;

public:
    //构造线条对象
    CLine();
    void AddPoint(const Point2D& pt);//追加顶点
    void SetSelected(bool sel);//设置选中状态
	bool IsSelected() const;//读取选中状态
	void SetColor(COLORREF color);//设置颜色
    COLORREF GetColor() const;//读取颜色
    void SetFill(bool hasFill, COLORREF fillColor);//设置填充属性
    bool HasFill() const;//是否填充
	COLORREF GetFillColor() const;//读取填充颜色
    void SetTextEntity(bool isTextEntity);
    bool IsTextEntity() const;//是否有text
    void SetTextContent(const std::wstring& text);//设置text
    const std::wstring& GetTextContent() const;//读取text
    void SetEntityType(EntityType type);//设置线条元数据
    EntityType GetEntityType() const;//读取线条元数据
    void SetEntityData(const EntityData& data);//设置几何元数据
    const EntityData& GetEntityData() const;//读取几何元数据

	const std::vector<Point2D>& GetPoints() const;//读取点集

    void Move(double dx, double dy);//平移line
    void Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const;//Draw line
    bool HitTest(const Point2D& pt, double tolerance) const;//鼠标是否命中
};
