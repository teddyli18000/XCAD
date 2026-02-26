#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"
#include "afxdialogex.h"

#include <afxdlgs.h>
#include <memory>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CCADDlg, CDialogEx)
    ON_WM_PAINT()
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
END_MESSAGE_MAP()

CCADDlg::CCADDlg(CWnd* pParent)
    : CDialogEx(IDD_CAD_DIALOG, pParent)
    , m_currentMode(CADMode::MODE_NONE)
    , m_bIsDrawing(false)
    , m_bIsPanning(false)
    , m_bShowPoints(true)
    , m_bLineCommandActive(false) {
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

void CCADDlg::OnPaint() {
    CPaintDC dc(this);

    CRect rect = m_transform.GetScreenRect();
    if (rect.IsRectEmpty()) return;

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBitmap);

    memDC.FillSolidRect(0, 0, rect.Width(), rect.Height(), RGB(33, 33, 33));

    m_shapeMgr.DrawAll(&memDC, m_transform, m_bShowPoints);

    if (m_bIsDrawing && m_pCurrentLine) {
        m_pCurrentLine->Draw(&memDC, m_transform, true);
    }

    dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void CCADDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);

    if (GetDlgItem(IDC_DRAW_AREA)) {
        CRect rect;
        GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&rect);
        ScreenToClient(&rect);
        m_transform.SetScreenRect(rect);
        RefreshCanvas();
    }
}

void CCADDlg::OnLButtonDown(UINT nFlags, CPoint point) {
    CRect rect = m_transform.GetScreenRect();
    if (!rect.PtInRect(point)) {
        CDialogEx::OnLButtonDown(nFlags, point);
        FocusCommandLine();
        return;
    }

    CPoint localPt(point.x - rect.left, point.y - rect.top);
    Point2D worldPt = m_transform.ScreenToWorld(localPt);

    if (m_currentMode == CADMode::MODE_DRAW && m_bLineCommandActive) {
        if (!m_bIsDrawing) {
            m_bIsDrawing = true;
            m_pCurrentLine = std::make_shared<CLine>();
            m_pCurrentLine->AddPoint(worldPt);
            m_pCurrentLine->AddPoint(worldPt);
        } else {
            m_pCurrentLine->AddPoint(worldPt);
        }
        RefreshCanvas();
    }

    FocusCommandLine();
}

void CCADDlg::OnMouseMove(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);

    if (m_bIsPanning) {
        m_transform.Pan(point.x - m_lastMousePt.x, point.y - m_lastMousePt.y);
        m_lastMousePt = point;
        RefreshCanvas();
        return;
    }

    if (m_bIsDrawing && m_pCurrentLine) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        Point2D worldPt = m_transform.ScreenToWorld(localPt);

        auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
        if (!pts.empty()) pts.back() = worldPt;
        RefreshCanvas();
    }
}

void CCADDlg::OnLButtonUp(UINT nFlags, CPoint point) {
    CDialogEx::OnLButtonUp(nFlags, point);
    FocusCommandLine();
}

void CCADDlg::FinishCurrentDrawing(bool keepCommandActive) {
    if (m_bIsDrawing && m_pCurrentLine) {
        auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
        if (pts.size() > 1) pts.pop_back();

        if (pts.size() >= 2) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, m_pCurrentLine));
        }
    }

    m_bIsDrawing = false;
    m_pCurrentLine.reset();
    m_bLineCommandActive = keepCommandActive;
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnRButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    FinishCurrentDrawing(false);
}

BOOL CCADDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
    ScreenToClient(&pt);
    CRect rect = m_transform.GetScreenRect();

    if (rect.PtInRect(pt)) {
        double factor = (zDelta > 0) ? 1.2 : 0.8;
        CPoint localPt(pt.x - rect.left, pt.y - rect.top);
        m_transform.Zoom(factor, localPt);
        RefreshCanvas();
    }

    FocusCommandLine();
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void CCADDlg::OnMButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    m_bIsPanning = true;
    m_lastMousePt = point;
    SetCapture();
    FocusCommandLine();
}

void CCADDlg::OnMButtonUp(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    m_bIsPanning = false;
    ReleaseCapture();
    FocusCommandLine();
}

void CCADDlg::CancelCurrentDrawing() {
    m_bIsDrawing = false;
    m_bLineCommandActive = false;
    m_pCurrentLine.reset();
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

void CCADDlg::ProcessCommandLine(const CString& cmd) {
    CString normalized = cmd;
    normalized.Trim();
    normalized.MakeUpper();

    if (normalized.IsEmpty()) return;

    if (normalized == _T("L") || normalized == _T("LINE") || normalized == _T("PL") || normalized == _T("PLINE")) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bLineCommandActive = true;
    } else if (normalized == _T("ESC") || normalized == _T("CANCEL")) {
        CancelCurrentDrawing();
    } else if (normalized == _T("U") || normalized == _T("UNDO")) {
        m_shapeMgr.Undo();
        RefreshCanvas();
    } else if (normalized == _T("REDO")) {
        m_shapeMgr.Redo();
        RefreshCanvas();
    } else if (normalized == _T("NEW")) {
        OnBnClickedNew2();
    } else if (normalized == _T("OPEN")) {
        OnBnClickedOpen();
    } else if (normalized == _T("SAVE") || normalized == _T("QSAVE")) {
        OnBnClickedSave();
    } else if (normalized == _T("SAVEAS") || normalized == _T("SAVE AS")) {
        OnBnClickedSaveAs();
    } else if (normalized == _T("SELECT") || normalized == _T("SEL")) {
        OnBnClickedSel();
    } else if (normalized == _T("DRAW")) {
        OnBnClickedDraw();
    } else if (normalized == _T("ZOOM") || normalized == _T("ZI")) {
        OnBnClickedZoomin();
    } else if (normalized == _T("ZO") || normalized == _T("ZOOMOUT")) {
        OnBnClickedZoomout();
    } else if (normalized == _T("ZE") || normalized == _T("ZOOMEXTENTS") || normalized == _T("ZOOM E")) {
        OnBnClickedZoomdef();
    } else if (normalized == _T("PAN") || normalized == _T("P")) {
        // 平移命令进入后用中键拖动/按钮平移
    } else if (normalized == _T("REGEN")) {
        RefreshCanvas();
    } else if (normalized == _T("POINTON") || normalized == _T("SHOWPOINT") || normalized == _T("SHOWPOINTS")) {
        OnBnClickedViewPoint();
    } else if (normalized == _T("POINTOFF") || normalized == _T("HIDEPOINT") || normalized == _T("HIDEPOINTS")) {
        OnBnClickedHidePoint();
    }

    FocusCommandLine();
}

BOOL CCADDlg::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_KEYDOWN) {
        const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

        if (ctrlDown && (pMsg->wParam == 'Z' || pMsg->wParam == 'z')) {
            m_shapeMgr.Undo();
            RefreshCanvas();
            FocusCommandLine();
            return TRUE;
        }

        if (ctrlDown && (pMsg->wParam == 'Y' || pMsg->wParam == 'y')) {
            m_shapeMgr.Redo();
            RefreshCanvas();
            FocusCommandLine();
            return TRUE;
        }

        if (pMsg->wParam == VK_ESCAPE) {
            CancelCurrentDrawing();
            return TRUE;
        }

        if (pMsg->wParam == VK_RETURN) {
            CWnd* pFocus = GetFocus();
            if (pFocus && pFocus->GetDlgCtrlID() == IDC_CMD_LINE) {
                CString cmd;
                pFocus->GetWindowText(cmd);
                ProcessCommandLine(cmd);
                pFocus->SetWindowText(_T(""));
                FocusCommandLine();
                return TRUE;
            }
        }
    }

    BOOL handled = CDialogEx::PreTranslateMessage(pMsg);

    if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_MBUTTONDOWN) {
        FocusCommandLine();
    }

    return handled;
}

void CCADDlg::OnBnClickedDraw() {
    m_currentMode = CADMode::MODE_DRAW;
    m_bLineCommandActive = true;
    FocusCommandLine();
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