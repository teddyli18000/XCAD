#include "pch.h"
#include "../CADDlg.h"

#include <memory>
#include <vector>

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

bool CCADDlg::HandleLineToolMouseMove(const Point2D& worldPt) {
    if (!(m_bIsDrawing && m_pCurrentLine)) return false;

    auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
    if (!pts.empty()) {
        pts.back() = worldPt;
        return true;
    }

    return false;
}
