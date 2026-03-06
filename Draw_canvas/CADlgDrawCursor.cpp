#include "pch.h"
#include "../CADDlg.h"

namespace {
const COLORREF kCursorColor = RGB(255, 255, 255);
const int kCursorLineWidth = 1;
const COLORREF kCrosshairColor = RGB(180, 180, 180);
const int kCrosshairRulerThickness = 20;
}

// 功能：绘制擦除/删段工具的圆形光标。
void CCADDlg::DrawCursor(CDC* pDC) {
    if (!pDC) return;

    CRect rect = m_transform.GetScreenRect();
    const int width = rect.Width();
    const int height = rect.Height();

    if (m_bMouseInCanvas && width > kCrosshairRulerThickness && height > kCrosshairRulerThickness) {
        CPen crossPen(PS_SOLID, 1, kCrosshairColor);
        CPen* oldCrossPen = pDC->SelectObject(&crossPen);
        pDC->MoveTo(m_mouseCanvasPt.x, kCrosshairRulerThickness);
        pDC->LineTo(m_mouseCanvasPt.x, height);
        pDC->MoveTo(0, m_mouseCanvasPt.y);
        pDC->LineTo(width - kCrosshairRulerThickness, m_mouseCanvasPt.y);
        pDC->SelectObject(oldCrossPen);
    }

    if ((m_bEraserCommandActive || m_bDeleteSegmentCommandActive || m_bInsertNodeCommandActive) && m_bEraserCursorVisible) {
        CPen pen(PS_SOLID, kCursorLineWidth, kCursorColor);
        CPen* oldPen = pDC->SelectObject(&pen);
        CBrush* oldBrush = static_cast<CBrush*>(pDC->SelectStockObject(NULL_BRUSH));
        pDC->Ellipse(m_eraserCursor.x - m_eraserRadius, m_eraserCursor.y - m_eraserRadius,
            m_eraserCursor.x + m_eraserRadius, m_eraserCursor.y + m_eraserRadius);
        pDC->SelectObject(oldBrush);
        pDC->SelectObject(oldPen);
    }
}
