#include "pch.h"
#include "../CADDlg.h"

//绘制当前模型中的全部图元
void CCADDlg::DrawModel(CDC* pDC) {
    if (!pDC) return;

    m_shapeMgr.DrawAll(pDC, m_transform, m_bShowPoints);
}
