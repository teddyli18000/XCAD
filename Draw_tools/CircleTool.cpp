#include "pch.h"
#include "../CADDlg.h"

#include <cmath>
#include <memory>

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
        if (radius > 0.0001) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateCirclePolyline(m_circleCenter, radius, 96)));
        }
        m_bCircleCenterPicked = false;
        m_bCircleCommandActive = false;
    }

    return true;
}

bool CCADDlg::HandleCircleToolMouseMove(const Point2D& worldPt) {
    if (!(m_bCircleCommandActive && m_bCircleCenterPicked)) return false;

    m_circlePreviewPoint = worldPt;
    return true;
}
