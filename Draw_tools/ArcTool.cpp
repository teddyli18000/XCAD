#include "pch.h"
#include "../CADDlg.h"

#include <memory>

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
        std::shared_ptr<CLine> arc = CreateArcPolylineByThreePoints(m_arcStartPoint, m_arcSecondPoint, worldPt, 120);
        if (arc->GetPoints().size() >= 2) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, arc));
        }
        m_arcPointCount = 0;
        m_bArcCommandActive = false;
    }

    return true;
}

bool CCADDlg::HandleArcToolMouseMove(const Point2D& worldPt) {
    if (!(m_bArcCommandActive && m_arcPointCount > 0)) return false;

    m_arcPreviewPoint = worldPt;
    return true;
}
