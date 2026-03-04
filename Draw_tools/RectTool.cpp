#include "pch.h"
#include "../CADDlg.h"

#include <memory>

bool CCADDlg::HandleRectToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bRectangleCommandActive)) return false;

    if (!m_bRectangleFirstPicked) {
        m_bRectangleFirstPicked = true;
        m_rectFirstPoint = worldPt;
        m_rectPreviewPoint = worldPt;
    } else {
        m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateRectanglePolyline(m_rectFirstPoint, worldPt)));
        m_bRectangleFirstPicked = false;
        m_bRectangleCommandActive = false;
    }

    return true;
}

bool CCADDlg::HandleRectToolMouseMove(const Point2D& worldPt) {
    if (!(m_bRectangleCommandActive && m_bRectangleFirstPicked)) return false;

    m_rectPreviewPoint = worldPt;
    return true;
}
