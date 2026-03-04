#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

#include <cmath>
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

    if (m_currentMode == CADMode::MODE_DRAW && m_bLineCommandActive) {
        if (!m_bIsDrawing) {
            m_bIsDrawing = true;
            m_pCurrentLine = std::make_shared<CLine>();
            m_pCurrentLine->AddPoint(worldPt);
            m_pCurrentLine->AddPoint(worldPt);
        } else {
            m_pCurrentLine->AddPoint(worldPt);
        }
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_DRAW && m_bCircleCommandActive) {
        if (!m_bCircleCenterPicked) {
            m_bCircleCenterPicked = true;
            m_circleCenter = worldPt;
            m_circlePreviewPoint = worldPt;
        } else {
            const double dx = worldPt.x - m_circleCenter.x;
            const double dy = worldPt.y - m_circleCenter.y;
            const double radius = std::sqrt(dx * dx + dy * dy);
            if (radius > 0.0001) {
                m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateCirclePolyline(m_circleCenter, radius, 96)));
            }
            m_bCircleCenterPicked = false;
            m_bCircleCommandActive = false;
        }
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_DRAW && m_bRectangleCommandActive) {
        if (!m_bRectangleFirstPicked) {
            m_bRectangleFirstPicked = true;
            m_rectFirstPoint = worldPt;
            m_rectPreviewPoint = worldPt;
        } else {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateRectanglePolyline(m_rectFirstPoint, worldPt)));
            m_bRectangleFirstPicked = false;
            m_bRectangleCommandActive = false;
        }
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_DRAW && m_bArcCommandActive) {
        if (m_arcPointCount == 0) {
            m_arcStartPoint = worldPt;
            m_arcPreviewPoint = worldPt;
            m_arcPointCount = 1;
        } else if (m_arcPointCount == 1) {
            m_arcSecondPoint = worldPt;
            m_arcPreviewPoint = worldPt;
            m_arcPointCount = 2;
        } else {
            std::shared_ptr<CLine> arc = CreateArcPolylineByThreePoints(m_arcStartPoint, m_arcSecondPoint, worldPt, 120);
            if (arc->GetPoints().size() >= 2) {
                m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, arc));
            }
            m_arcPointCount = 0;
            m_bArcCommandActive = false;
        }
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_SELECT) {
        ClearSelection();
        if (m_bEraserCommandActive) {
            m_bIsErasing = true;
            m_eraserCursor = localPt;
            m_bEraserCursorVisible = true;
            EraseAtPoint(localPt);
        } else {
            m_bIsSelectingBox = true;
            m_selectBoxStart = localPt;
            m_selectBoxEnd = localPt;
        }
        SetCapture();
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

    if (m_bIsDrawing && m_pCurrentLine) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        Point2D worldPt = m_transform.ScreenToWorld(localPt);

        auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
        if (!pts.empty()) pts.back() = worldPt;
        RefreshCanvas();
        return;
    }

    if (m_bCircleCommandActive && m_bCircleCenterPicked) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        m_circlePreviewPoint = m_transform.ScreenToWorld(localPt);
        RefreshCanvas();
        return;
    }

    if (m_bRectangleCommandActive && m_bRectangleFirstPicked) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        m_rectPreviewPoint = m_transform.ScreenToWorld(localPt);
        RefreshCanvas();
        return;
    }

    if (m_bArcCommandActive && m_arcPointCount > 0) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        m_arcPreviewPoint = m_transform.ScreenToWorld(localPt);
        RefreshCanvas();
        return;
    }

    CRect rect = m_transform.GetScreenRect();
    bool inCanvas = rect.PtInRect(point);
    CPoint localPt(point.x - rect.left, point.y - rect.top);

    if (m_bEraserCommandActive) {
        m_bEraserCursorVisible = inCanvas;
        if (inCanvas) {
            m_eraserCursor = localPt;
            if (m_bIsErasing && (nFlags & MK_LBUTTON)) {
                EraseAtPoint(localPt);
            }
        }
        RefreshCanvas();
        return;
    }

    if (m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox) {
        m_selectBoxEnd = localPt;
        RefreshCanvas();
    }
}

void CCADDlg::OnLButtonUp(UINT nFlags, CPoint point) {
    CRect rect = m_transform.GetScreenRect();
    CPoint localPt(point.x - rect.left, point.y - rect.top);

    if (m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox) {
        m_selectBoxEnd = localPt;
        ApplySelectionBox();
        m_bIsSelectingBox = false;
        if (GetCapture() == this) {
            ReleaseCapture();
        }
        RefreshCanvas();
    }

    if (m_bEraserCommandActive && m_bIsErasing) {
        m_bIsErasing = false;
        if (GetCapture() == this) {
            ReleaseCapture();
        }
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
