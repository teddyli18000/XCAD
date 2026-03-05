#include "pch.h"
#include "../CADDlg.h"

#include <cmath>

namespace {
const double kMoveEpsilon = 1e-9;
}

//开始框选流程，记录起始点并捕获鼠标
bool CCADDlg::HandleSelectionToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && !m_bEraserCommandActive && !m_bDeleteNodeCommandActive && !m_bHatchCommandActive)) return false;

    if (HasSelectedLines()) {
        m_bIsMovingSelection = true;
        m_selectionMoveLastPt = localPt;
        m_selectionMoveTotalDx = 0.0;
        m_selectionMoveTotalDy = 0.0;
        m_selectionMoveShapes.clear();
        for (const auto& shape : m_shapeMgr.GetShapes()) {
            if (shape && shape->IsSelected()) {
                m_selectionMoveShapes.push_back(shape);
            }
        }
        SetCapture();
        return true;
    }

    m_bIsSelectingBox = true;
    m_selectBoxStart = localPt;
    m_selectBoxEnd = localPt;
    SetCapture();
    return true;
}

//更新框选终点，实现拖拽框实时预览
bool CCADDlg::HandleSelectionToolMouseMove(const CPoint& localPt) {
    if (m_currentMode == CADMode::MODE_SELECT && m_bIsMovingSelection) {
        Point2D prevWorld = m_transform.ScreenToWorld(m_selectionMoveLastPt);
        Point2D nowWorld = m_transform.ScreenToWorld(localPt);
        const double dx = nowWorld.x - prevWorld.x;
        const double dy = nowWorld.y - prevWorld.y;
        m_selectionMoveLastPt = localPt;

        if (std::fabs(dx) <= kMoveEpsilon && std::fabs(dy) <= kMoveEpsilon) {
            return false;
        }

        for (const auto& shape : m_selectionMoveShapes) {
            if (shape) {
                shape->Move(dx, dy);
            }
        }
        m_selectionMoveTotalDx += dx;
        m_selectionMoveTotalDy += dy;
        return true;
    }

    if (!(m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox)) return false;

    m_selectBoxEnd = localPt;
    return true;
}

//结束框选流程，应用框选结果并释放鼠标捕获
bool CCADDlg::HandleSelectionToolLButtonUp(const CPoint& localPt) {
    if (m_currentMode == CADMode::MODE_SELECT && m_bIsMovingSelection) {
        m_bIsMovingSelection = false;
        if (GetCapture() == this) {
            ReleaseCapture();
        }

        if (std::fabs(m_selectionMoveTotalDx) <= kMoveEpsilon && std::fabs(m_selectionMoveTotalDy) <= kMoveEpsilon) {
            ClearSelection();
            m_selectionMoveShapes.clear();
            return true;
        }

        m_shapeMgr.ExecuteCommand(std::make_unique<CMoveLinesCommand>(&m_shapeMgr, m_selectionMoveShapes,
            m_selectionMoveTotalDx, m_selectionMoveTotalDy, true));
        m_selectionMoveShapes.clear();
        return true;
    }

    if (!(m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox)) return false;

    m_selectBoxEnd = localPt;
    ApplySelectionBox();
    m_bIsSelectingBox = false;
    if (GetCapture() == this) {
        ReleaseCapture();
    }
    return true;
}
