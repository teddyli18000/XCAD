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
// return: true:已处理矩形对角点输入并可能完成建形;
// false:当前未处于矩形绘制状态;

//处理鼠标移动，更新对角点预览
bool CCADDlg::HandleRectToolMouseMove(const Point2D& worldPt) {
    if (!(m_bRectangleCommandActive && m_bRectangleFirstPicked)) return false;

    m_rectPreviewPoint = worldPt;
    return true;
}
// return: true:已更新矩形预览点;
// false:当前无有效矩形预览状态;
