#include "pch.h"
#include "CLine.h"
#include "CViewTransform.h"

#include <cmath>

namespace {
const COLORREF kCadColorWhite = RGB(255, 255, 255);
const COLORREF kCadColorBlue = RGB(0, 0, 255);
const int kLineWidth = 1;//可扩展：用户设置线宽
const int kPointMarkerHalfSize = 3;
const int kTextInset = 2;
const int kTextMinPixelHeight = 14;//text
const int kTextMaxPixelHeight = 72;//text
const double kPointEqualEpsilon = 1e-9;//比较阈值
const int kMinClosedPointCount = 3;//闭合图形至少点数（用于填充）

bool IsClosedPolylinePoints(const std::vector<Point2D>& points) {
    if (points.size() < kMinClosedPointCount) {
        return false;
    }
    const Point2D& a = points.front();
    const Point2D& b = points.back();
    return std::fabs(a.x - b.x) <= kPointEqualEpsilon && std::fabs(a.y - b.y) <= kPointEqualEpsilon;
}
}

//构造线条对象并初始化默认显示属性
CLine::CLine() : m_bSelected(false), m_color(kCadColorWhite), m_hasFill(false), m_fillColor(kCadColorWhite), m_isTextEntity(false), m_textContent(), m_entityType(EntityType::LINE), m_entityData() {}

//在线条末尾追加一个顶点
void CLine::AddPoint(const Point2D& pt) { m_points.push_back(pt); }

//设置线条选中状态
void CLine::SetSelected(bool sel) { m_bSelected = sel; }

//读取线条选中状态
bool CLine::IsSelected() const { return m_bSelected; }

//设置线条颜色
void CLine::SetColor(COLORREF color) { m_color = color; }

//读取线条颜色
COLORREF CLine::GetColor() const { return m_color; }

//设置填充状态
void CLine::SetFill(bool hasFill, COLORREF fillColor) {
    m_hasFill = hasFill;
    m_fillColor = fillColor;
}

//读取是否填充
bool CLine::HasFill() const { return m_hasFill; }

//读取填充颜色
COLORREF CLine::GetFillColor() const { return m_fillColor; }

//设置是否为文本图元
void CLine::SetTextEntity(bool isTextEntity) { m_isTextEntity = isTextEntity; }

//读取是否为文本图元
bool CLine::IsTextEntity() const { return m_isTextEntity; }

//设置文本内容
void CLine::SetTextContent(const std::wstring& text) { m_textContent = text; }

//读取文本内容
const std::wstring& CLine::GetTextContent() const { return m_textContent; }

// 设置图元语义类型
void CLine::SetEntityType(EntityType type) { m_entityType = type; }

// 读取图元语义类型
EntityType CLine::GetEntityType() const { return m_entityType; }

// 设置图元参数元数据
void CLine::SetEntityData(const EntityData& data) { m_entityData = data; }

// 读取图元参数元数据
const EntityData& CLine::GetEntityData() const { return m_entityData; }

//读取线条点集(read-only)
const std::vector<Point2D>& CLine::GetPoints() const { return m_points; }

//整体平移线条点集
void CLine::Move(double dx, double dy) {
    for (auto& pt : m_points) {
        pt.x += dx;
        pt.y += dy;
    }

    if (m_entityType == EntityType::CIRCLE || m_entityType == EntityType::ARC) {
        m_entityData.Center.x += dx;
        m_entityData.Center.y += dy;
    }
}

//绘制线条、填充、文字与顶点标记
void CLine::Draw(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const {
    if (m_points.empty()) return;

    if (m_isTextEntity) {
        CRect textRect;
        textRect.left = LONG_MAX;
        textRect.top = LONG_MAX;
        textRect.right = LONG_MIN;
        textRect.bottom = LONG_MIN;

        for (const auto& p : m_points) {
            const CPoint spt = transform.WorldToScreen(p);
            if (spt.x < textRect.left) textRect.left = spt.x;
            if (spt.x > textRect.right) textRect.right = spt.x;
            if (spt.y < textRect.top) textRect.top = spt.y;
            if (spt.y > textRect.bottom) textRect.bottom = spt.y;
        }

        if (textRect.left > textRect.right || textRect.top > textRect.bottom) return;

        if (m_bSelected) {
            CPen borderPen(PS_DASH, kLineWidth, m_color);
            CBrush* oldBrush = static_cast<CBrush*>(pDC->SelectStockObject(NULL_BRUSH));
            CPen* oldPen = pDC->SelectObject(&borderPen);
            pDC->Rectangle(&textRect);
            pDC->SelectObject(oldPen);
            pDC->SelectObject(oldBrush);
        }

        CRect drawRect = textRect;
        drawRect.DeflateRect(kTextInset, kTextInset);

        int fontHeightPx = (drawRect.Height() * 3) / 4;
        if (fontHeightPx < kTextMinPixelHeight) fontHeightPx = kTextMinPixelHeight;
        if (fontHeightPx > kTextMaxPixelHeight) fontHeightPx = kTextMaxPixelHeight;

        CFont textFont;
        textFont.CreateFontW(-fontHeightPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
        CFont* oldFont = pDC->SelectObject(&textFont);

        const int oldBkMode = pDC->SetBkMode(TRANSPARENT);
        const COLORREF oldTextColor = pDC->SetTextColor(m_color);
        pDC->DrawTextW(m_textContent.c_str(), -1, &drawRect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
        pDC->SetTextColor(oldTextColor);
        pDC->SetBkMode(oldBkMode);
        pDC->SelectObject(oldFont);

        if (bShowPoints || m_bSelected) {
            CBrush brush(kCadColorBlue);
            CBrush* pOldBrush = pDC->SelectObject(&brush);
            for (const auto& pt : m_points) {
                CPoint spt = transform.WorldToScreen(pt);
                pDC->Rectangle(spt.x - kPointMarkerHalfSize, spt.y - kPointMarkerHalfSize,
                    spt.x + kPointMarkerHalfSize, spt.y + kPointMarkerHalfSize);
            }
            pDC->SelectObject(pOldBrush);
        }
        return;
    }

    if (m_hasFill && IsClosedPolylinePoints(m_points)) {
        std::vector<CPoint> screenPts;
        screenPts.reserve(m_points.size());
        for (const auto& p : m_points) {
            screenPts.push_back(transform.WorldToScreen(p));
        }
        if (screenPts.size() >= 3) {
            CBrush fillBrush(m_fillColor);
            CBrush* oldBrush = pDC->SelectObject(&fillBrush);
            CPen* oldPen = static_cast<CPen*>(pDC->SelectStockObject(NULL_PEN));
            pDC->Polygon(screenPts.data(), static_cast<int>(screenPts.size()));
            pDC->SelectObject(oldPen);
            pDC->SelectObject(oldBrush);
        }
    }

    CPen pen(m_bSelected ? PS_DASH : PS_SOLID, kLineWidth, m_color);
    CPen* pOldPen = pDC->SelectObject(&pen);

    CPoint startPt = transform.WorldToScreen(m_points[0]);
    pDC->MoveTo(startPt);

    for (size_t i = 1; i < m_points.size(); ++i) {
        pDC->LineTo(transform.WorldToScreen(m_points[i]));
    }

    pDC->SelectObject(pOldPen);

    if (bShowPoints || m_bSelected) {
        CBrush brush(kCadColorBlue);
        CBrush* pOldBrush = pDC->SelectObject(&brush);
        for (const auto& pt : m_points) {
            CPoint spt = transform.WorldToScreen(pt);
            pDC->Rectangle(spt.x - kPointMarkerHalfSize, spt.y - kPointMarkerHalfSize,
                spt.x + kPointMarkerHalfSize, spt.y + kPointMarkerHalfSize);
        }
        pDC->SelectObject(pOldBrush);
    }
}

//命中测试占位（当前固定返回 false）
bool CLine::HitTest(const Point2D& pt, double tolerance) const {
    UNREFERENCED_PARAMETER(pt);
    UNREFERENCED_PARAMETER(tolerance);
    return false;
}
