#include "pch.h"
#include "../CADDlg.h"
#include "../resource.h"

#include <memory>

namespace {
const int kTextEditInset = 2;
const int kTextEditMinWidth = 32;
const int kTextEditMinHeight = 20;
}

// 功能：处理文字工具左键点击，依次确定文本框两个对角点。
bool CCADDlg::HandleTextToolLButtonDown(const Point2D& worldPt) {
    if (!(m_currentMode == CADMode::MODE_DRAW && m_bTextCommandActive)) return false;

    if (!m_bTextFirstPicked) {
        m_bTextFirstPicked = true;
        m_textFirstPoint = worldPt;
        m_textPreviewPoint = worldPt;
    } else {
        std::shared_ptr<CLine> textShape = CreateRectanglePolyline(m_textFirstPoint, worldPt);
        textShape->SetTextEntity(true);
        textShape->SetTextContent(L"");

        m_bTextFirstPicked = false;
        m_bTextCommandActive = false;
        BeginTextInput(textShape);
    }

    return true;
}

// 功能：处理文字工具鼠标移动，更新文本框预览。
bool CCADDlg::HandleTextToolMouseMove(const Point2D& worldPt) {
    if (!(m_bTextCommandActive && m_bTextFirstPicked)) return false;

    m_textPreviewPoint = worldPt;
    return true;
}

// 功能：在画布文本框区域创建输入控件。
void CCADDlg::BeginTextInput(const std::shared_ptr<CLine>& textShape) {
    if (!textShape) return;

    CommitTextInput(false);

    const auto& pts = textShape->GetPoints();
    if (pts.size() < 2) return;

    CPoint p1 = m_transform.WorldToScreen(pts[0]);
    CPoint p3 = m_transform.WorldToScreen(pts[2]);
    CRect editRect(min(p1.x, p3.x), min(p1.y, p3.y), max(p1.x, p3.x), max(p1.y, p3.y));

    if (editRect.Width() < kTextEditMinWidth) editRect.right = editRect.left + kTextEditMinWidth;
    if (editRect.Height() < kTextEditMinHeight) editRect.bottom = editRect.top + kTextEditMinHeight;
    editRect.DeflateRect(kTextEditInset, kTextEditInset);

    CRect canvasRect = m_transform.GetScreenRect();
    editRect.OffsetRect(canvasRect.left, canvasRect.top);

    const DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;
    if (!m_textInputEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), style, editRect, this, IDC_TEXT_INLINE_EDIT)) {
        return;
    }

    m_textInputEdit.SetFont(GetFont());
    m_textInputEdit.SetFocus();
    m_textInputEdit.SetSel(0, -1);

    m_pendingTextShape = textShape;
    m_bTextInputActive = true;
    UpdateModeButtonHighlight();
}

// 功能：提交或取消当前文本输入。
void CCADDlg::CommitTextInput(bool acceptInput) {
    if (!m_bTextInputActive) return;

    CString text;
    if (m_textInputEdit.GetSafeHwnd()) {
        m_textInputEdit.GetWindowText(text);
        m_textInputEdit.DestroyWindow();
    }

    if (acceptInput && m_pendingTextShape && !text.IsEmpty()) {
        m_pendingTextShape->SetTextContent(std::wstring(text.GetString()));
        m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, m_pendingTextShape));
    }

    m_pendingTextShape.reset();
    m_bTextInputActive = false;
    UpdateModeButtonHighlight();
}
