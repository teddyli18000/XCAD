#include "pch.h"
#include "../CADDlg.h"

#include <memory>

//处理左键点击，依次确定两个对角点
bool CCADDlg::HandleRectToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bRectangleCommandActive)) return false;

    if (!m_bRectangleFirstPicked) {
        m_bRectangleFirstPicked = true;
        m_rectFirstPoint = worldPt;
        m_rectPreviewPoint = worldPt;
    } else {
        m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateRectanglePolyline(m_rectFirstPoint, worldPt)));
        m_bRectangleFirstPicked = false;
    }

    return true;
}

//处理鼠标移动，更新对角点预览
bool CCADDlg::HandleRectToolMouseMove(const Point2D& worldPt) {
    if (!(m_bRectangleCommandActive && m_bRectangleFirstPicked)) return false;

    m_rectPreviewPoint = worldPt;
    return true;
}
