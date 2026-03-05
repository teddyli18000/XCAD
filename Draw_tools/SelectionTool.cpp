#include "pch.h"
#include "../CADDlg.h"

bool CCADDlg::HandleSelectionToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && !m_bEraserCommandActive && !m_bDeleteNodeCommandActive)) return false;

    m_bIsSelectingBox = true;
    m_selectBoxStart = localPt;
    m_selectBoxEnd = localPt;
    SetCapture();
    return true;
}

bool CCADDlg::HandleSelectionToolMouseMove(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox)) return false;

    m_selectBoxEnd = localPt;
    return true;
}

bool CCADDlg::HandleSelectionToolLButtonUp(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox)) return false;

    m_selectBoxEnd = localPt;
    ApplySelectionBox();
    m_bIsSelectingBox = false;
    if (GetCapture() == this) {
        ReleaseCapture();
    }
    return true;
}
