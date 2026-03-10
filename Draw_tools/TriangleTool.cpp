#include "pch.h"
#include "../CADDlg.h"

#include <memory>
#include <vector>

bool CCADDlg::HandleTriangleToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bTriangleCommandActive)) return false;

    if (!m_bTriangleFirstPicked) {
        m_bTriangleFirstPicked = true;
        m_triangleFirstPoint = worldPt;
        m_trianglePreviewPoint = worldPt;
    }
    else if (!m_bTriangleSecondPicked) {
        m_bTriangleSecondPicked = true;
        m_triangleSecondPoint = worldPt;
    }
    else {
        m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateTrianglePolyline(m_triangleFirstPoint, m_triangleSecondPoint, worldPt)));
        m_bTriangleFirstPicked = false;
        m_bTriangleSecondPicked = false;
        m_trianglePreviewPoint = worldPt;
    }

    return true;
}

bool CCADDlg::HandleTriangleToolMouseMove(const Point2D& worldPt) {
    if (!(m_bTriangleCommandActive && m_bTriangleFirstPicked)) return false;
    m_trianglePreviewPoint = worldPt;
    return true;
}
