#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"
#include "afxdialogex.h"

#include <afxdlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif  

// 主对话框模块 / main dialog module
// 下面函数在本文件被调用前的用法说明：
// - `CreateCirclePolyline(center, radius, segments)`:
//   center=圆心(world), radius=半径(world), segments=离散段数(>=8)
// - `CreateRectanglePolyline(first, second)`:
//   first/second=对角点(world), 返回闭合矩形折线
// - `CreateArcPolylineByThreePoints(start, through, end, segments)`:
//   start=起点, through=经过点, end=终点, segments=插值段数
// - `RefreshCanvas()`:
//   仅刷新绘图区，不重绘整个对话框

BEGIN_MESSAGE_MAP(CCADDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_DRAWITEM()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_MBUTTONDOWN()
    ON_WM_MBUTTONUP()
    ON_WM_MOUSEWHEEL()
    ON_BN_CLICKED(IDC_DRAW, &CCADDlg::OnBnClickedDraw)
    ON_BN_CLICKED(IDC_DRAW_LINE, &CCADDlg::OnBnClickedDraw)
    ON_BN_CLICKED(IDC_DRAW_CIRCLE, &CCADDlg::OnBnClickedCircle)
    ON_BN_CLICKED(IDC_DRAW_RECT, &CCADDlg::OnBnClickedRectangle)
    ON_BN_CLICKED(IDC_DRAW_ARC, &CCADDlg::OnBnClickedArc)
    ON_BN_CLICKED(IDC_SEL, &CCADDlg::OnBnClickedSel)
    ON_BN_CLICKED(IDC_SELECT, &CCADDlg::OnBnClickedSel)
    ON_BN_CLICKED(IDC_VIEW_POINT, &CCADDlg::OnBnClickedViewPoint)
    ON_BN_CLICKED(IDC_HIDE_POINT, &CCADDlg::OnBnClickedHidePoint)
    ON_BN_CLICKED(IDC_ZOOMIN, &CCADDlg::OnBnClickedZoomin)
    ON_BN_CLICKED(IDC_ZOOMOUT, &CCADDlg::OnBnClickedZoomout)
    ON_BN_CLICKED(IDC_ZOOMDEF, &CCADDlg::OnBnClickedZoomdef)
    ON_BN_CLICKED(IDC_MUP, &CCADDlg::OnBnClickedMup)
    ON_BN_CLICKED(IDC_MDOWN, &CCADDlg::OnBnClickedMdown)
    ON_BN_CLICKED(IDC_ML, &CCADDlg::OnBnClickedMl)
    ON_BN_CLICKED(IDC_MR, &CCADDlg::OnBnClickedMr)
    ON_BN_CLICKED(IDC_OPEN, &CCADDlg::OnBnClickedOpen)
    ON_BN_CLICKED(IDC_NEW2, &CCADDlg::OnBnClickedNew2)
    ON_BN_CLICKED(IDC_SAVE, &CCADDlg::OnBnClickedSave)
    ON_BN_CLICKED(IDC_SAVE_AS, &CCADDlg::OnBnClickedSaveAs)
    ON_BN_CLICKED(IDC_UNDO, &CCADDlg::OnBnClickedUndo)
    ON_BN_CLICKED(IDC_REDO, &CCADDlg::OnBnClickedRedo)
    ON_BN_CLICKED(IDC_DEL_LINE, &CCADDlg::OnBnClickedDelLine)
    ON_BN_CLICKED(IDC_ABOUT_ICON, &CCADDlg::OnBnClickedAboutIcon)
END_MESSAGE_MAP()

CCADDlg::CCADDlg(CWnd* pParent)
    : CDialogEx(IDD_CAD_DIALOG, pParent)
    , m_currentMode(CADMode::MODE_NONE)
    , m_bIsDrawing(false)
    , m_bIsPanning(false)
    , m_bShowPoints(true)
    , m_bLineCommandActive(false)
    , m_bCircleCommandActive(false)
    , m_bCircleCenterPicked(false)
    , m_bRectangleCommandActive(false)
    , m_bRectangleFirstPicked(false)
    , m_bArcCommandActive(false)
    , m_bEraserCommandActive(false)
    , m_bIsSelectingBox(false)
    , m_bIsErasing(false)
    , m_bEraserCursorVisible(false)
    , m_arcPointCount(0)
    , m_eraserRadius(18)
    , m_selectBoxStart(0, 0)
    , m_selectBoxEnd(0, 0)
    , m_eraserCursor(0, 0)
    , m_circleCenter(0.0, 0.0)
    , m_circlePreviewPoint(0.0, 0.0)
    , m_rectFirstPoint(0.0, 0.0)
    , m_rectPreviewPoint(0.0, 0.0)
    , m_arcStartPoint(0.0, 0.0)
    , m_arcSecondPoint(0.0, 0.0)
    , m_arcPreviewPoint(0.0, 0.0) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCADDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BOOL CCADDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    if (GetDlgItem(IDC_DRAW_AREA)) {
        CRect rect;
        GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&rect);
        ScreenToClient(&rect);
        m_transform.SetScreenRect(rect);
        GetDlgItem(IDC_DRAW_AREA)->ShowWindow(SW_HIDE);
    }

    FocusCommandLine();
    return FALSE;
}

void CCADDlg::OnSetFocus(CWnd* pOldWnd) {
    CDialogEx::OnSetFocus(pOldWnd);
    FocusCommandLine();
}

void CCADDlg::FocusCommandLine() {
    CWnd* cmd = GetDlgItem(IDC_CMD_LINE);
    if (!cmd || !::IsWindow(cmd->GetSafeHwnd())) return;

    if (GetFocus() != cmd) {
        cmd->SetFocus();
    }

    CEdit* edit = dynamic_cast<CEdit*>(cmd);
    if (edit) {
        int len = edit->GetWindowTextLength();
        edit->SetSel(len, len);
    }
}

void CCADDlg::RefreshCanvas() {
    CRect rect = m_transform.GetScreenRect();
    if (!rect.IsRectEmpty()) {
        InvalidateRect(&rect, FALSE);
    }
}

void CCADDlg::ActivateCommand(CADCommandType commandType) {
    ClearSelection();
    m_bLineCommandActive = false;
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bArcCommandActive = false;
    m_bEraserCommandActive = false;
    m_bIsSelectingBox = false;
    m_bIsErasing = false;
    m_bEraserCursorVisible = false;
    m_arcPointCount = 0;
    m_pCurrentLine.reset();
    m_bIsDrawing = false;

    if (commandType == CADCommandType::LINE) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bLineCommandActive = true;
    } else if (commandType == CADCommandType::CIRCLE) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bCircleCommandActive = true;
    } else if (commandType == CADCommandType::RECTANGLE) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bRectangleCommandActive = true;
    } else if (commandType == CADCommandType::ARC) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bArcCommandActive = true;
    } else if (commandType == CADCommandType::ERASER) {
        m_currentMode = CADMode::MODE_SELECT;
        m_bEraserCommandActive = true;
    }

    RefreshCanvas();
    FocusCommandLine();
}

bool CCADDlg::SaveToCurrentPath() {
    if (m_currentFilePath.IsEmpty()) return false;

    bool ok = m_shapeMgr.SaveToDXF(std::wstring(m_currentFilePath.GetString()));
    if (!ok) {
        AfxMessageBox(_T("Save failed."));
    }
    return ok;
}

bool CCADDlg::SaveAsWithDialog() {
    CFileDialog dlg(FALSE, L"dxf", nullptr,
        OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
        _T("DXF Files (*.dxf)|*.dxf|All Files (*.*)|*.*||"), this);

    if (dlg.DoModal() != IDOK) return false;

    m_currentFilePath = dlg.GetPathName();
    return SaveToCurrentPath();
}

void CCADDlg::OnBnClickedDraw() {
    ActivateCommand(CADCommandType::LINE);
}

void CCADDlg::OnBnClickedCircle() {
    ActivateCommand(CADCommandType::CIRCLE);
}

void CCADDlg::OnBnClickedRectangle() {
    ActivateCommand(CADCommandType::RECTANGLE);
}

void CCADDlg::OnBnClickedArc() {
    ActivateCommand(CADCommandType::ARC);
}

void CCADDlg::OnBnClickedSel() {
    m_currentMode = CADMode::MODE_SELECT;
    CancelCurrentDrawing();
}

void CCADDlg::OnBnClickedViewPoint() {
    m_bShowPoints = true;
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedHidePoint() {
    m_bShowPoints = false;
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedZoomin() {
    CRect rect = m_transform.GetScreenRect();
    CPoint center(rect.Width() / 2, rect.Height() / 2);
    m_transform.Zoom(1.2, center);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedZoomout() {
    CRect rect = m_transform.GetScreenRect();
    CPoint center(rect.Width() / 2, rect.Height() / 2);
    m_transform.Zoom(0.8, center);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedZoomdef() {
    CRect rect = m_transform.GetScreenRect();
    m_transform = CViewTransform();
    m_transform.SetScreenRect(rect);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedMup() {
    m_transform.Pan(0, -20);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedMdown() {
    m_transform.Pan(0, 20);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedMl() {
    m_transform.Pan(-20, 0);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedMr() {
    m_transform.Pan(20, 0);
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedOpen() {
    CFileDialog dlg(TRUE, L"dxf", nullptr,
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("DXF Files (*.dxf)|*.dxf|All Files (*.*)|*.*||"), this);

    if (dlg.DoModal() != IDOK) {
        FocusCommandLine();
        return;
    }

    CString path = dlg.GetPathName();
    if (!m_shapeMgr.LoadFromDXF(std::wstring(path.GetString()))) {
        AfxMessageBox(_T("Open failed."));
        FocusCommandLine();
        return;
    }

    m_currentFilePath = path;
    CancelCurrentDrawing();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedNew2() {
    CFileDialog dlg(FALSE, L"dxf", nullptr,
        OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
        _T("DXF Files (*.dxf)|*.dxf|All Files (*.*)|*.*||"), this);

    if (dlg.DoModal() != IDOK) {
        FocusCommandLine();
        return;
    }

    m_currentFilePath = dlg.GetPathName();
    m_shapeMgr.Clear();
    CancelCurrentDrawing();
    SaveToCurrentPath();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedSave() {
    if (m_currentFilePath.IsEmpty()) {
        OnBnClickedNew2();
        return;
    }

    SaveToCurrentPath();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedSaveAs() {
    SaveAsWithDialog();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedUndo() {
    m_shapeMgr.Undo();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedRedo() {
    m_shapeMgr.Redo();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnBnClickedDelLine() {
    ActivateCommand(CADCommandType::ERASER);
}

void CCADDlg::OnBnClickedAboutIcon() {
    CDialogEx aboutDlg(IDD_ABOUTBOX, this);
    aboutDlg.DoModal();
    FocusCommandLine();
}