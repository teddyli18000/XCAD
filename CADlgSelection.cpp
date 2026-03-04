#include "pch.h"
#include "CADDlg.h"
#include "CADlgGeometryUtils.h"

#include <memory>
#include <vector>

// 选择与擦除 / selection + eraser
// localPt: 画布局部坐标 / canvas local coordinate

void CCADDlg::ClearSelection() {
    for (auto& shape : m_shapeMgr.GetShapes()) {
        shape->SetSelected(false);
    }
}

void CCADDlg::ApplySelectionBox() {
    if (m_currentMode != CADMode::MODE_SELECT || m_bEraserCommandActive) return;

    const CRect box = cad::dlg::NormalizeRect(m_selectBoxStart, m_selectBoxEnd);
    if (box.Width() < 2 && box.Height() < 2) {
        ClearSelection();
        return;
    }

    ClearSelection();
    for (auto& shape : m_shapeMgr.GetShapes()) {
        if (cad::dlg::PolylineIntersectsRect(*shape, box, m_transform)) {
            shape->SetSelected(true);
        }
    }
}

void CCADDlg::DeleteSelectedLines() {
    std::vector<std::shared_ptr<CLine>> selected;
    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (shape->IsSelected()) {
            selected.push_back(shape);
        }
    }

    if (selected.empty()) return;
    m_shapeMgr.ExecuteCommand(std::make_unique<CDeleteLinesCommand>(&m_shapeMgr, std::move(selected)));
    RefreshCanvas();
}

void CCADDlg::EraseAtPoint(const CPoint& localPt) {
    if (!m_bEraserCommandActive) return;

    std::vector<std::shared_ptr<CLine>> hits;
    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (cad::dlg::PolylineIntersectsCircle(*shape, localPt, m_eraserRadius, m_transform)) {
            hits.push_back(shape);
        }
    }

    if (!hits.empty()) {
        m_shapeMgr.ExecuteCommand(std::make_unique<CDeleteLinesCommand>(&m_shapeMgr, std::move(hits)));
        RefreshCanvas();
    }
}
