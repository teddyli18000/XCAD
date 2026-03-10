#include "pch.h"
#include "../CADDlg.h"

#include <memory>
#include <vector>

//处理左键点击，创建或追加折线节点
bool CCADDlg::HandleLineToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bLineCommandActive)) return false;

    if (!m_bIsDrawing) {
        m_bIsDrawing = true;
        m_pCurrentLine = std::make_shared<CLine>();
        m_pCurrentLine->AddPoint(worldPt);
        m_pCurrentLine->AddPoint(worldPt);
    } else {
        m_pCurrentLine->AddPoint(worldPt);
    }

    return true;
}
// return: true:已处理折线节点输入;
// false:当前未处于线段绘制状态;

//处理鼠标移动，更新最后一个预览点
bool CCADDlg::HandleLineToolMouseMove(const Point2D& worldPt) {
    if (!(m_bIsDrawing && m_pCurrentLine)) return false;

    auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
    if (!pts.empty()) {
        pts.back() = worldPt;
        return true;
    }

    return false;
}
// return: true:已更新折线预览末点;
// false:当前无可更新的绘制状态;
