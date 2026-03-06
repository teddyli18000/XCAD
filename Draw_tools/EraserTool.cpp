#include "pch.h"
#include "../CADDlg.h"

//开始擦除/删点流程并立即执行一次命中处理
bool CCADDlg::HandleEraserToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && (m_bEraserCommandActive || m_bDeleteSegmentCommandActive))) return false;

    m_bIsErasing = true;
    m_eraserCursor = localPt;
    m_bEraserCursorVisible = true;
    EraseAtPoint(localPt);
    SetCapture();
    return true;
}

//更新橡皮擦光标位置，并在拖动时持续擦除
bool CCADDlg::HandleEraserToolMouseMove(UINT nFlags, const CPoint& localPt, bool inCanvas) {
    if (!(m_bEraserCommandActive || m_bDeleteSegmentCommandActive)) return false;

    m_bEraserCursorVisible = inCanvas;
    if (inCanvas) {
        m_eraserCursor = localPt;
        if (m_bIsErasing && (nFlags & MK_LBUTTON)) {
            EraseAtPoint(localPt);
        }
    }
    return true;
}

//结束擦除流程并释放鼠标捕获
bool CCADDlg::HandleEraserToolLButtonUp() {
    if (!((m_bEraserCommandActive || m_bDeleteSegmentCommandActive) && m_bIsErasing)) return false;

    m_bIsErasing = false;
    if (GetCapture() == this) {
        ReleaseCapture();
    }
    return true;
}
