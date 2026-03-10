#include "pch.h"
#include "../CADDlg.h"

#include <cmath>

namespace {
const double kMoveEpsilon = 1e-9;
}

//开始框选流程，记录起始点并捕获鼠标
bool CCADDlg::HandleSelectionToolLButtonDown(const CPoint& localPt) {
    if (!(m_currentMode == CADMode::MODE_SELECT && !m_bEraserCommandActive && !m_bDeleteSegmentCommandActive && !m_bInsertNodeCommandActive && !m_bHatchCommandActive)) return false;

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
// return: true:已开始移动选中集或框选流程;
// false:当前不满足选择工具处理条件;

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
// return: true:已处理移动选中集或框选预览更新;
// false:当前无可处理的选择动作;

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
// return: true:已结束移动或框选并完成结果应用;
// false:当前无有效选择结束动作;