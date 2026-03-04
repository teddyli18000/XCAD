#include "pch.h"
#include "../CADDlg.h"

bool CCADDlg::HandleEraserToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && m_bEraserCommandActive)) return false;

    m_bIsErasing = true;
    m_eraserCursor = localPt;
    m_bEraserCursorVisible = true;
    EraseAtPoint(localPt);
    SetCapture();
    return true;
}

bool CCADDlg::HandleEraserToolMouseMove(UINT nFlags, const CPoint& localPt, bool inCanvas) {
    if (!m_bEraserCommandActive) return false;

    m_bEraserCursorVisible = inCanvas;
    if (inCanvas) {
        m_eraserCursor = localPt;
        if (m_bIsErasing && (nFlags & MK_LBUTTON)) {
            EraseAtPoint(localPt);
        }
    }
    return true;
}

bool CCADDlg::HandleEraserToolLButtonUp() {
    if (!(m_bEraserCommandActive && m_bIsErasing)) return false;

    m_bIsErasing = false;
    if (GetCapture() == this) {
        ReleaseCapture();
    }
    return true;
}
