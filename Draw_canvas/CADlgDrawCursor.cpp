#include "pch.h"
#include "../CADDlg.h"

void CCADDlg::DrawCursor(CDC* pDC) {
    if (!pDC) return;

    if ((m_bEraserCommandActive || m_bDeleteNodeCommandActive) && m_bEraserCursorVisible) {
        CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
        CPen* oldPen = pDC->SelectObject(&pen);
        CBrush* oldBrush = static_cast<CBrush*>(pDC->SelectStockObject(NULL_BRUSH));
        pDC->Ellipse(m_eraserCursor.x - m_eraserRadius, m_eraserCursor.y - m_eraserRadius,
            m_eraserCursor.x + m_eraserRadius, m_eraserCursor.y + m_eraserRadius);
        pDC->SelectObject(oldBrush);
        pDC->SelectObject(oldPen);
    }
}
