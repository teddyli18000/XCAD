#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

#include <memory>
#include <vector>

// 输入模块 / input module

void CCADDlg::OnLButtonDown(UINT nFlags, CPoint point) {
    CRect rect = m_transform.GetScreenRect();
    if (!rect.PtInRect(point)) {
        CDialogEx::OnLButtonDown(nFlags, point);
        FocusCommandLine();
        return;
    }

    CPoint localPt(point.x - rect.left, point.y - rect.top);
    Point2D worldPt = m_transform.ScreenToWorld(localPt);
    bool handled = false;

    if (m_currentMode == CADMode::MODE_DRAW) {
        if (m_bLineCommandActive) {
            handled = HandleLineToolLButtonDown(worldPt);
        } else if (m_bCircleCommandActive) {
            handled = HandleCircleToolLButtonDown(worldPt);
        } else if (m_bRectangleCommandActive) {
            handled = HandleRectToolLButtonDown(worldPt);
        } else if (m_bArcCommandActive) {
            handled = HandleArcToolLButtonDown(worldPt);
        }
    } else if (m_currentMode == CADMode::MODE_SELECT) {
        ClearSelection();
        if (m_bEraserCommandActive) {
            handled = HandleEraserToolLButtonDown(localPt);
        } else {
            handled = HandleSelectionToolLButtonDown(localPt);
        }
    }

    if (handled) {
        RefreshCanvas();
    }

    FocusCommandLine();
}

void CCADDlg::OnMouseMove(UINT nFlags, CPoint point) {
    if (m_bIsPanning) {
        m_transform.Pan(point.x - m_lastMousePt.x, point.y - m_lastMousePt.y);
        m_lastMousePt = point;
        RefreshCanvas();
        return;
    }

    CRect rect = m_transform.GetScreenRect();
    bool inCanvas = rect.PtInRect(point);
    CPoint localPt(point.x - rect.left, point.y - rect.top);
    Point2D worldPt = m_transform.ScreenToWorld(localPt);

    if (HandleLineToolMouseMove(worldPt)
        || HandleCircleToolMouseMove(worldPt)
        || HandleRectToolMouseMove(worldPt)
        || HandleArcToolMouseMove(worldPt)
        || HandleEraserToolMouseMove(nFlags, localPt, inCanvas)
        || HandleSelectionToolMouseMove(localPt)) {
        RefreshCanvas();
        return;
    }
}

void CCADDlg::OnLButtonUp(UINT nFlags, CPoint point) {
    CRect rect = m_transform.GetScreenRect();
    CPoint localPt(point.x - rect.left, point.y - rect.top);

    if (HandleSelectionToolLButtonUp(localPt) || HandleEraserToolLButtonUp()) {
        RefreshCanvas();
    }

    CDialogEx::OnLButtonUp(nFlags, point);
    FocusCommandLine();
}

void CCADDlg::OnRButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    if (m_bIsDrawing && m_bLineCommandActive) {
        FinishCurrentDrawing(false);
    } else {
        CancelActiveCommand();
    }
}

BOOL CCADDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
    ScreenToClient(&pt);
    CRect rect = m_transform.GetScreenRect();

    if (rect.PtInRect(pt)) {
        double factor = (zDelta > 0) ? 1.2 : 0.8;
        CPoint localPt(pt.x - rect.left, pt.y - rect.top);
        m_transform.Zoom(factor, localPt);
        RefreshCanvas();
    }

    FocusCommandLine();
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void CCADDlg::OnMButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    m_bIsPanning = true;
    m_lastMousePt = point;
    SetCapture();
    FocusCommandLine();
}

void CCADDlg::OnMButtonUp(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    m_bIsPanning = false;
    ReleaseCapture();
    FocusCommandLine();
}

void CCADDlg::FinishCurrentDrawing(bool keepCommandActive) {
    if (m_bIsDrawing && m_pCurrentLine) {
        auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
        if (pts.size() > 1) pts.pop_back();

        if (pts.size() >= 2) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, m_pCurrentLine));
        }
    }

    m_bIsDrawing = false;
    m_pCurrentLine.reset();
    m_bLineCommandActive = keepCommandActive;
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bArcCommandActive = false;
    m_arcPointCount = 0;
    UpdateModeButtonHighlight();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::CancelCurrentDrawing() {
    m_bIsDrawing = false;
    m_bLineCommandActive = false;
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bArcCommandActive = false;
    m_bEraserCommandActive = false;
    m_bIsSelectingBox = false;
    m_bIsErasing = false;
    m_bEraserCursorVisible = false;
    m_arcPointCount = 0;
    m_pCurrentLine.reset();
    if (GetCapture() == this) {
        ReleaseCapture();
    }
    ClearSelection();
    UpdateModeButtonHighlight();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::CancelActiveCommand() {
    if (m_bIsDrawing && m_bLineCommandActive && m_pCurrentLine) {
        auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
        if (pts.size() > 1) pts.pop_back();
        if (pts.size() >= 2) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, m_pCurrentLine));
        }
    }

    m_bIsDrawing = false;
    m_pCurrentLine.reset();
    m_bLineCommandActive = false;
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bArcCommandActive = false;
    m_bEraserCommandActive = false;
    m_bIsSelectingBox = false;
    m_bIsErasing = false;
    m_bEraserCursorVisible = false;
    m_arcPointCount = 0;

    if (GetCapture() == this) {
        ReleaseCapture();
    }

    UpdateModeButtonHighlight();
    RefreshCanvas();
    FocusCommandLine();
}
