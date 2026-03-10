#include "pch.h"
#include "../CADDlg.h"

#include <memory>

namespace {
const int kArcBuildSegments = 120;//精度
}

//处理左键点击，按三点法生成圆弧
bool CCADDlg::HandleArcToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bArcCommandActive)) return false;

    if (m_arcPointCount == 0) {
        m_arcStartPoint = worldPt;
        m_arcPreviewPoint = worldPt;
        m_arcPointCount = 1;
    } else if (m_arcPointCount == 1) {
        m_arcSecondPoint = worldPt;
        m_arcPreviewPoint = worldPt;
        m_arcPointCount = 2;
    } else {
        std::shared_ptr<CLine> arc = CreateArcPolylineByThreePoints(m_arcStartPoint, m_arcSecondPoint, worldPt, kArcBuildSegments);
        if (arc->GetPoints().size() >= 2) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, arc));
        }
        m_arcPointCount = 0;
    }

    return true;
}
// return: true:已处理圆弧三点输入流程;
// false:当前未处于圆弧绘制状态;

//处理鼠标移动，更新预览终点
bool CCADDlg::HandleArcToolMouseMove(const Point2D& worldPt) {
    if (!(m_bArcCommandActive && m_arcPointCount > 0)) return false;

    m_arcPreviewPoint = worldPt;
    return true;
}
// return: true:已更新圆弧预览点; 
// false:无有效圆弧预览状态;
