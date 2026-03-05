#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

#include <memory>
#include <vector>

// 输入模块 / input module

namespace {
const double kZoomInFactor = 1.2;
const double kZoomOutFactor = 0.8;
}

// 功能：处理鼠标左键按下，按当前模式分发到对应工具。
// 交互步骤（LButtonDown）：
// 1) 先判断点击是否在画布内（in canvas）。
// 2) 坐标从屏幕坐标转换成局部/世界坐标（screen -> local/world）。
// 3) 根据当前模式把事件交给具体工具处理。
// 4) 若工具返回 handled，则只刷新画布区域。
void CCADDlg::OnLButtonDown(UINT nFlags, CPoint point) {
    if (m_bTextInputActive) {
        CommitTextInput(true);
        RefreshCanvas();
        FocusCommandLine();
        return;
    }

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
        } else if (m_bTextCommandActive) {
            handled = HandleTextToolLButtonDown(worldPt);
        } else if (m_bArcCommandActive) {
            handled = HandleArcToolLButtonDown(worldPt);
        }
    } else if (m_currentMode == CADMode::MODE_SELECT) {
        if (m_bHatchCommandActive) {
            handled = HandleHatchToolLButtonDown(localPt);
        } else {
            if (m_bEraserCommandActive || m_bDeleteNodeCommandActive) {
                handled = HandleEraserToolLButtonDown(localPt);
            } else {
                handled = HandleSelectionToolLButtonDown(localPt);
            }
        }
    }

    if (handled) {
        RefreshCanvas();
    }

    FocusCommandLine();
}

//处理鼠标移动，包括平移和各绘图工具的预览更新
void CCADDlg::OnMouseMove(UINT nFlags, CPoint point) {
    const bool prevMouseInCanvas = m_bMouseInCanvas;
    const CPoint prevMouseCanvasPt = m_mouseCanvasPt;

    if (m_bIsPanning) {
        CRect panRect = m_transform.GetScreenRect();
        m_bMouseInCanvas = panRect.PtInRect(point);
        if (m_bMouseInCanvas) {
            m_mouseCanvasPt = CPoint(point.x - panRect.left, point.y - panRect.top);
        }

        m_transform.Pan(point.x - m_lastMousePt.x, point.y - m_lastMousePt.y);
        m_lastMousePt = point;
        RefreshCanvas();
        return;
    }

    CRect rect = m_transform.GetScreenRect();
    bool inCanvas = rect.PtInRect(point);
    CPoint localPt(point.x - rect.left, point.y - rect.top);
    Point2D worldPt = m_transform.ScreenToWorld(localPt);

    m_bMouseInCanvas = inCanvas;
    if (inCanvas) {
        m_mouseCanvasPt = localPt;
    }

    if (HandleLineToolMouseMove(worldPt)
        || HandleCircleToolMouseMove(worldPt)
        || HandleRectToolMouseMove(worldPt)
        || HandleTextToolMouseMove(worldPt)
        || HandleArcToolMouseMove(worldPt)
        || HandleHatchToolMouseMove(localPt, inCanvas)
        || HandleEraserToolMouseMove(nFlags, localPt, inCanvas)
        || HandleSelectionToolMouseMove(localPt)) {
        RefreshCanvas();
        return;
    }

    if (prevMouseInCanvas != m_bMouseInCanvas
        || (m_bMouseInCanvas
            && (prevMouseCanvasPt.x != m_mouseCanvasPt.x || prevMouseCanvasPt.y != m_mouseCanvasPt.y))) {
        RefreshCanvas();
    }
}

//处理鼠标左键释放，完成框选或擦除状态收尾
void CCADDlg::OnLButtonUp(UINT nFlags, CPoint point) {
    CRect rect = m_transform.GetScreenRect();
    CPoint localPt(point.x - rect.left, point.y - rect.top);

    if (HandleSelectionToolLButtonUp(localPt) || HandleEraserToolLButtonUp()) {
        RefreshCanvas();
    }

    CDialogEx::OnLButtonUp(nFlags, point);
    FocusCommandLine();
}

//处理鼠标右键，结束当前绘制或取消当前命令
void CCADDlg::OnRButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    if (m_bIsDrawing && m_bLineCommandActive) {
        FinishCurrentDrawing(false);
    } else {
        CancelActiveCommand();
    }
}

//处理滚轮缩放，以鼠标位置作为缩放中心
BOOL CCADDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
    ScreenToClient(&pt);
    CRect rect = m_transform.GetScreenRect();

    if (rect.PtInRect(pt)) {
        double factor = (zDelta > 0) ? kZoomInFactor : kZoomOutFactor;
        CPoint localPt(pt.x - rect.left, pt.y - rect.top);
        m_transform.Zoom(factor, localPt);
        RefreshCanvas();
    }

    FocusCommandLine();
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

//处理中键按下，进入平移拖动模式
void CCADDlg::OnMButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    m_bIsPanning = true;
    m_lastMousePt = point;
    SetCapture();
    FocusCommandLine();
}

//处理中键抬起，退出平移拖动模式
void CCADDlg::OnMButtonUp(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    m_bIsPanning = false;
    ReleaseCapture();
    FocusCommandLine();
}

//结束当前折线绘制，并按条件提交命令
void CCADDlg::FinishCurrentDrawing(bool keepCommandActive) {
    CommitTextInput(true);
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
    m_bTextCommandActive = false;
    m_bTextFirstPicked = false;
    m_bArcCommandActive = false;
    m_arcPointCount = 0;
    UpdateModeButtonHighlight();
    RefreshCanvas();
    FocusCommandLine();
}

//取消当前绘制及相关工具状态
void CCADDlg::CancelCurrentDrawing() {
    CommitTextInput(false);
    m_bIsDrawing = false;
    m_bLineCommandActive = false;
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bTextCommandActive = false;
    m_bTextFirstPicked = false;
    m_bArcCommandActive = false;
    m_bHatchCommandActive = false;
    m_bEraserCommandActive = false;
    m_bDeleteNodeCommandActive = false;
    m_bHatchPreviewVisible = false;
    m_bIsSelectingBox = false;
    m_bIsMovingSelection = false;
    m_bIsErasing = false;
    m_bEraserCursorVisible = false;
    m_selectionMoveTotalDx = 0.0;
    m_selectionMoveTotalDy = 0.0;
    m_selectionMoveShapes.clear();
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

//取消当前激活命令，必要时保留已画有效线段
void CCADDlg::CancelActiveCommand() {
    CommitTextInput(false);
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
    m_bTextCommandActive = false;
    m_bTextFirstPicked = false;
    m_bArcCommandActive = false;
    m_bHatchCommandActive = false;
    m_bEraserCommandActive = false;
    m_bDeleteNodeCommandActive = false;
    m_bHatchPreviewVisible = false;
    m_bIsSelectingBox = false;
    m_bIsMovingSelection = false;
    m_bIsErasing = false;
    m_bEraserCursorVisible = false;
    m_selectionMoveTotalDx = 0.0;
    m_selectionMoveTotalDy = 0.0;
    m_selectionMoveShapes.clear();
    m_arcPointCount = 0;

    if (GetCapture() == this) {
        ReleaseCapture();
    }

    UpdateModeButtonHighlight();
    RefreshCanvas();
    FocusCommandLine();
}
