#include "pch.h"
#include "../CADDlg.h"

#include <cmath>
#include <memory>

namespace {
const double kCircleRadiusMin = 0.0001;
const int kCircleSegments = 96;//精度
}

//处理左键点击，依次确定圆心与半径
bool CCADDlg::HandleCircleToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bCircleCommandActive)) return false;

    if (!m_bCircleCenterPicked) {
        m_bCircleCenterPicked = true;
        m_circleCenter = worldPt;
        m_circlePreviewPoint = worldPt;
    } else {
        const double dx = worldPt.x - m_circleCenter.x;
        const double dy = worldPt.y - m_circleCenter.y;
        const double radius = std::sqrt(dx * dx + dy * dy);
        if (radius > kCircleRadiusMin) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateCirclePolyline(m_circleCenter, radius, kCircleSegments)));
        }
        m_bCircleCenterPicked = false;
    }

    return true;
}
// return: true:已处理圆心/半径输入并可能完成建圆;
// false:未处于圆绘制状态;

//处理鼠标移动，更新半径预览点
bool CCADDlg::HandleCircleToolMouseMove(const Point2D& worldPt) {
    if (!(m_bCircleCommandActive && m_bCircleCenterPicked)) return false;

    m_circlePreviewPoint = worldPt;
    return true;
}
// return: true:已更新圆半径预览点;
// false:无有效圆预览状态;
