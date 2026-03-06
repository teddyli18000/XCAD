#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"
#include "afxdialogex.h"

#include <afxdlgs.h>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif  

namespace {
const COLORREF kCadColorWhite = RGB(255, 255, 255);
const COLORREF kCadColorRed = RGB(255, 0, 0);
const COLORREF kCadColorYellow = RGB(255, 255, 0);
const COLORREF kCadColorGreen = RGB(0, 255, 0);
const COLORREF kCadColorCyan = RGB(0, 255, 255);
const COLORREF kCadColorBlue = RGB(0, 0, 255);
const COLORREF kCadColorMagenta = RGB(255, 0, 255);
const COLORREF kCadColorBlack = RGB(0, 0, 0);

const int kColorButtonMinDisplaySize = 12;
const int kColorButtonInnerMargin = 1;
const int kColorButtonSizeThreshold = 4;

const int kPanStepPixel = 20;
const int kEraserRadiusDefault = 18;

const double kZoomInFactor = 1.2;
const double kZoomOutFactor = 0.8;

void SetButtonPushedState(CCADDlg* pDlg, int ctrlId, bool bPushed) {
    CWnd* pButton = pDlg->GetDlgItem(ctrlId);
    if (!pButton || !::IsWindow(pButton->GetSafeHwnd())) {
        return;
    }
    pButton->SendMessage(BM_SETSTATE, bPushed ? TRUE : FALSE, 0);
}
}

// 主对话框模块 / main dialog module
// 函数用法说明：
// CreateLinePolyline(start, end):
// start/end=起止点(world)，返回一条折线（虽然只有一个线段）
// 
// CreateCirclePolyline(center, radius, segments):
// center=圆心(world), radius=半径(world), segments=离散段数(>=8)
//
// CreateRectanglePolyline(first, second):
// first/second=对角点(world), 返回闭合矩形折线
// 
// CreateArcPolylineByThreePoints(start, through, end, segments):
// start=起点, through=经过点, end=终点, segments=插值段数
// 
// CreateTextShape(position, text):
// position=文字基点(world), text=字符串内容
// 
// RefreshCanvas():（修改过，保证无bug）
// 仅刷新绘图区，不重绘整个对话框

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
    ON_BN_CLICKED(IDC_DRAW_TEXT, &CCADDlg::OnBnClickedText)
    ON_BN_CLICKED(IDC_DRAW_ARC, &CCADDlg::OnBnClickedArc)
    ON_BN_CLICKED(IDC_HATCH, &CCADDlg::OnBnClickedHatch)
    ON_BN_CLICKED(IDC_SEL, &CCADDlg::OnBnClickedSel)
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
    ON_BN_CLICKED(IDC_DEL_SEGMENT, &CCADDlg::OnBnClickedDelSegment)
    ON_BN_CLICKED(IDC_INSERT_NODE, &CCADDlg::OnBnClickedInsertNode)
    ON_BN_CLICKED(IDC_DEL_LINE, &CCADDlg::OnBnClickedDelLine)
    ON_BN_CLICKED(IDC_COLOR_WHITE, &CCADDlg::OnBnClickedColorWhite)
    ON_BN_CLICKED(IDC_COLOR_RED, &CCADDlg::OnBnClickedColorRed)
    ON_BN_CLICKED(IDC_COLOR_YELLOW, &CCADDlg::OnBnClickedColorYellow)
    ON_BN_CLICKED(IDC_COLOR_GREEN, &CCADDlg::OnBnClickedColorGreen)
    ON_BN_CLICKED(IDC_COLOR_CYAN, &CCADDlg::OnBnClickedColorCyan)
    ON_BN_CLICKED(IDC_COLOR_BLUE, &CCADDlg::OnBnClickedColorBlue)
    ON_BN_CLICKED(IDC_COLOR_MAGENTA, &CCADDlg::OnBnClickedColorMagenta)
    ON_BN_CLICKED(IDC_ABOUT_ICON, &CCADDlg::OnBnClickedAboutIcon)
    ON_STN_CLICKED(IDC_DRAW_AREA, &CCADDlg::OnStnClickedDrawArea)
END_MESSAGE_MAP()

//构造主对话框并初始化所有运行状态
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
    , m_bTextCommandActive(false)
    , m_bTextFirstPicked(false)
    , m_bTextInputActive(false)
    , m_bArcCommandActive(false)
    , m_bHatchCommandActive(false)
    , m_bEraserCommandActive(false)
    , m_bDeleteSegmentCommandActive(false)
    , m_bInsertNodeCommandActive(false)
    , m_bHatchPreviewVisible(false)
    , m_bIsSelectingBox(false)
    , m_bIsMovingSelection(false)
    , m_bIsErasing(false)
    , m_bEraserCursorVisible(false)
    , m_bMouseInCanvas(false)
    , m_arcPointCount(0)
    , m_eraserRadius(kEraserRadiusDefault)
    , m_selectBoxStart(0, 0)
    , m_selectBoxEnd(0, 0)
    , m_selectionMoveLastPt(0, 0)
    , m_eraserCursor(0, 0)
    , m_mouseCanvasPt(0, 0)
    , m_hatchPreviewPoint(0, 0)
    , m_circleCenter(0.0, 0.0)
    , m_circlePreviewPoint(0.0, 0.0)
    , m_rectFirstPoint(0.0, 0.0)
    , m_rectPreviewPoint(0.0, 0.0)
    , m_textFirstPoint(0.0, 0.0)
    , m_textPreviewPoint(0.0, 0.0)
    , m_arcStartPoint(0.0, 0.0)
    , m_arcSecondPoint(0.0, 0.0)
    , m_arcPreviewPoint(0.0, 0.0)
    , m_hatchColor(kCadColorWhite)
    , m_selectionMoveTotalDx(0.0)
    , m_selectionMoveTotalDy(0.0) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

//完成控件与成员变量的数据交换
void CCADDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

//初始化主界面、绘图区和颜色按钮资源
BOOL CCADDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    if (GetDlgItem(IDC_DRAW_AREA)) {
        CRect rect;
        GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&rect);
        ScreenToClient(&rect);
        m_transform.SetScreenRect(rect);
        GetDlgItem(IDC_DRAW_AREA)->ShowWindow(SW_HIDE);
    }

    struct ColorButtonDef { int ctrlId; COLORREF color; };
    const ColorButtonDef colorButtons[] = {
        { IDC_COLOR_WHITE, kCadColorWhite },
        { IDC_COLOR_RED, kCadColorRed },
        { IDC_COLOR_YELLOW, kCadColorYellow },
        { IDC_COLOR_GREEN, kCadColorGreen },
        { IDC_COLOR_CYAN, kCadColorCyan },
        { IDC_COLOR_BLUE, kCadColorBlue },
        { IDC_COLOR_MAGENTA, kCadColorMagenta }
    };

    m_colorButtonBitmaps.clear();
    m_colorButtonBitmaps.reserve(_countof(colorButtons));
    CClientDC clientDC(this);
    for (const auto& def : colorButtons) {
        CWnd* button = GetDlgItem(def.ctrlId);
        if (!button || !::IsWindow(button->GetSafeHwnd())) continue;

        CRect rc;
        button->GetClientRect(&rc);
        const int w = (rc.Width() > kColorButtonSizeThreshold) ? rc.Width() : kColorButtonMinDisplaySize;
        const int h = (rc.Height() > kColorButtonSizeThreshold) ? rc.Height() : kColorButtonMinDisplaySize;

        auto bmp = std::make_unique<CBitmap>();
        if (!bmp->CreateCompatibleBitmap(&clientDC, w, h)) continue;

        CDC memDC;
        memDC.CreateCompatibleDC(&clientDC);
        CBitmap* oldBmp = memDC.SelectObject(bmp.get());

        memDC.FillSolidRect(0, 0, w, h, GetSysColor(COLOR_3DFACE));
        CRect square(kColorButtonInnerMargin, kColorButtonInnerMargin, w - kColorButtonInnerMargin, h - kColorButtonInnerMargin);
        memDC.FillSolidRect(&square, def.color);
        memDC.Draw3dRect(&square, kCadColorBlack, kCadColorBlack);

        memDC.SelectObject(oldBmp);

        button->ModifyStyle(BS_TYPEMASK, BS_BITMAP);
        button->SetWindowText(_T(""));
        button->SendMessage(BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bmp->GetSafeHandle()));
        button->ShowWindow(SW_SHOW);
        button->Invalidate();

        m_colorButtonBitmaps.push_back(std::move(bmp));
    }

    UpdateModeButtonHighlight();
    UpdateFileInfoDisplay();
    UpdateFileInfoLayout();
    FocusCommandLine();
    return FALSE;
}

void CCADDlg::UpdateFileInfoDisplay() {
    CString filePathText;
    CString fileSizeText;

    if (!m_currentFilePath.IsEmpty()) {
        filePathText = _T("本地文件：") + m_currentFilePath;

        CFileStatus status;
        if (CFile::GetStatus(m_currentFilePath, status)) {
            fileSizeText.Format(_T("文件大小：%I64u 字节"), static_cast<ULONGLONG>(status.m_size));
        }
    }

    CWnd* pPath = GetDlgItem(IDC_FILE_PATH_INFO);
    if (pPath && ::IsWindow(pPath->GetSafeHwnd())) {
        pPath->SetWindowText(filePathText);
    }

    CWnd* pSize = GetDlgItem(IDC_FILE_SIZE_INFO);
    if (pSize && ::IsWindow(pSize->GetSafeHwnd())) {
        pSize->SetWindowText(fileSizeText);
    }
}

void CCADDlg::UpdateFileInfoLayout() {
    CWnd* pCmd = GetDlgItem(IDC_CMD_LINE);
    CWnd* pPath = GetDlgItem(IDC_FILE_PATH_INFO);
    CWnd* pSize = GetDlgItem(IDC_FILE_SIZE_INFO);
    if (!pCmd || !pPath || !pSize) {
        return;
    }
    if (!::IsWindow(pCmd->GetSafeHwnd()) || !::IsWindow(pPath->GetSafeHwnd()) || !::IsWindow(pSize->GetSafeHwnd())) {
        return;
    }

    CRect cmdRect;
    pCmd->GetWindowRect(&cmdRect);
    ScreenToClient(&cmdRect);

    CRect dlgRect;
    GetClientRect(&dlgRect);

    int top = cmdRect.bottom;
    int height = dlgRect.bottom - top - 4;
    if (height < 14) {
        height = 14;
    }

    const int totalWidth = cmdRect.Width();
    int rightWidth = 220;
    if (totalWidth < 440) {
        rightWidth = totalWidth / 2;
    }

    const int leftWidth = totalWidth - rightWidth;

    pPath->SetWindowPos(nullptr, cmdRect.left, top, leftWidth, height, SWP_NOZORDER | SWP_NOACTIVATE);
    pSize->SetWindowPos(nullptr, cmdRect.left + leftWidth, top, rightWidth, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

//根据当前命令状态更新按钮高亮显示
void CCADDlg::UpdateModeButtonHighlight() {
    const bool lineActive = (m_currentMode == CADMode::MODE_DRAW && m_bLineCommandActive);
    const bool circleActive = (m_currentMode == CADMode::MODE_DRAW && m_bCircleCommandActive);
    const bool rectActive = (m_currentMode == CADMode::MODE_DRAW && m_bRectangleCommandActive);
    const bool textActive = (m_currentMode == CADMode::MODE_DRAW && m_bTextCommandActive);
    const bool arcActive = (m_currentMode == CADMode::MODE_DRAW && m_bArcCommandActive);
    const bool hatchActive = (m_currentMode == CADMode::MODE_SELECT && m_bHatchCommandActive);
    const bool drawModeActive = lineActive || circleActive || rectActive || textActive || arcActive || hatchActive;
    const bool eraseActive = (m_currentMode == CADMode::MODE_SELECT && m_bEraserCommandActive);
    const bool deleteSegmentActive = (m_currentMode == CADMode::MODE_SELECT && m_bDeleteSegmentCommandActive);
    const bool insertNodeActive = (m_currentMode == CADMode::MODE_SELECT && m_bInsertNodeCommandActive);
    const bool selectModeActive = (m_currentMode == CADMode::MODE_SELECT && !m_bEraserCommandActive && !m_bDeleteSegmentCommandActive && !m_bInsertNodeCommandActive && !m_bHatchCommandActive);

    SetButtonPushedState(this, IDC_DRAW, drawModeActive);
    SetButtonPushedState(this, IDC_DRAW_LINE, lineActive);
    SetButtonPushedState(this, IDC_DRAW_CIRCLE, circleActive);
    SetButtonPushedState(this, IDC_DRAW_RECT, rectActive);
    SetButtonPushedState(this, IDC_DRAW_TEXT, textActive);
    SetButtonPushedState(this, IDC_DRAW_ARC, arcActive);
    SetButtonPushedState(this, IDC_HATCH, hatchActive);
    SetButtonPushedState(this, IDC_DEL_SEGMENT, deleteSegmentActive);
    SetButtonPushedState(this, IDC_INSERT_NODE, insertNodeActive);
    SetButtonPushedState(this, IDC_SEL, selectModeActive);
    SetButtonPushedState(this, IDC_DEL_LINE, eraseActive);
}

//窗口获得焦点时，把输入焦点切到命令行
void CCADDlg::OnSetFocus(CWnd* pOldWnd) {
    CDialogEx::OnSetFocus(pOldWnd);
    FocusCommandLine();
}

//将焦点定位到命令行输入框，并把光标放到末尾
void CCADDlg::FocusCommandLine() {
    if (m_bTextInputActive) return;

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

//只刷新绘图区，避免整窗重绘
void CCADDlg::RefreshCanvas() {
    CRect rect = m_transform.GetScreenRect();
    if (!rect.IsRectEmpty()) {
        InvalidateRect(&rect, FALSE);
    }
}

//激活指定命令并重置其他工具状态
void CCADDlg::ActivateCommand(CADCommandType commandType) {
    CommitTextInput(true);
    ClearSelection();
    m_bLineCommandActive = false;
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bTextCommandActive = false;
    m_bTextFirstPicked = false;
    m_bArcCommandActive = false;
    m_bHatchCommandActive = false;
    m_bEraserCommandActive = false;
    m_bDeleteSegmentCommandActive = false;
    m_bInsertNodeCommandActive = false;
    m_bHatchPreviewVisible = false;
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
    } else if (commandType == CADCommandType::TEXT) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bTextCommandActive = true;
    } else if (commandType == CADCommandType::ARC) {
        m_currentMode = CADMode::MODE_DRAW;
        m_bArcCommandActive = true;
    } else if (commandType == CADCommandType::HATCH) {
        m_currentMode = CADMode::MODE_SELECT;
        m_bHatchCommandActive = true;
    } else if (commandType == CADCommandType::ERASER) {
        m_currentMode = CADMode::MODE_SELECT;
        m_bEraserCommandActive = true;
    } else if (commandType == CADCommandType::DELETE_SEGMENT) {
        m_currentMode = CADMode::MODE_SELECT;
        m_bDeleteSegmentCommandActive = true;
    } else if (commandType == CADCommandType::INSERT_NODE) {
        m_currentMode = CADMode::MODE_SELECT;
        m_bInsertNodeCommandActive = true;
    }

    UpdateModeButtonHighlight();
    RefreshCanvas();
    FocusCommandLine();
}

//保存到当前文件路径
bool CCADDlg::SaveToCurrentPath() {
    if (m_currentFilePath.IsEmpty()) return false;

    bool ok = m_shapeMgr.SaveToDXF(std::wstring(m_currentFilePath.GetString()));
    if (!ok) {
        AfxMessageBox(_T("Save failed."));
    } else {
        m_shapeMgr.MarkSaved();
    }
    UpdateFileInfoDisplay();
    return ok;
}

//弹出另存为对话框并保存
bool CCADDlg::SaveAsWithDialog() {
    CFileDialog dlg(FALSE, L"dxf", nullptr,
        OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
        _T("DXF Files (*.dxf)|*.dxf|All Files (*.*)|*.*||"), this);

    if (dlg.DoModal() != IDOK) return false;

    m_currentFilePath = dlg.GetPathName();
    const bool saved = SaveToCurrentPath();
    UpdateFileInfoDisplay();
    return saved;
}

//激活line绘制命令
void CCADDlg::OnBnClickedDraw() {
    ActivateCommand(CADCommandType::LINE);
}

//激活circle绘制命令
void CCADDlg::OnBnClickedCircle() {
    ActivateCommand(CADCommandType::CIRCLE);
}

//激活rectangle绘制命令
void CCADDlg::OnBnClickedRectangle() {
    ActivateCommand(CADCommandType::RECTANGLE);
}

//激活text绘制命令
void CCADDlg::OnBnClickedText() {
    ActivateCommand(CADCommandType::TEXT);
}

//激活arc绘制命令
void CCADDlg::OnBnClickedArc() {
    ActivateCommand(CADCommandType::ARC);
}

//激活hatch命令
void CCADDlg::OnBnClickedHatch() {
    ActivateCommand(CADCommandType::HATCH);
}

//切换到sel模式
void CCADDlg::OnBnClickedSel() {
    m_currentMode = CADMode::MODE_SELECT;
    CancelCurrentDrawing();
    UpdateModeButtonHighlight();
}

//显示顶点可视点
void CCADDlg::OnBnClickedViewPoint() {
    m_bShowPoints = true;
    RefreshCanvas();
    FocusCommandLine();
}

//隐藏顶点可视点
void CCADDlg::OnBnClickedHidePoint() {
    m_bShowPoints = false;
    RefreshCanvas();
    FocusCommandLine();
}

//以画布中心为基准放大视图
void CCADDlg::OnBnClickedZoomin() {
    CRect rect = m_transform.GetScreenRect();
    CPoint center(rect.Width() / 2, rect.Height() / 2);
    m_transform.Zoom(kZoomInFactor, center);
    RefreshCanvas();
    FocusCommandLine();
}

//以画布中心为基准缩小视图
void CCADDlg::OnBnClickedZoomout() {
    CRect rect = m_transform.GetScreenRect();
    CPoint center(rect.Width() / 2, rect.Height() / 2);
    m_transform.Zoom(kZoomOutFactor, center);
    RefreshCanvas();
    FocusCommandLine();
}

//重置视图变换到默认状态
void CCADDlg::OnBnClickedZoomdef() {
    CRect rect = m_transform.GetScreenRect();
    m_transform = CViewTransform();
    m_transform.SetScreenRect(rect);
    RefreshCanvas();
    FocusCommandLine();
}

//视图向上平移固定像素
void CCADDlg::OnBnClickedMup() {
    m_transform.Pan(0, -kPanStepPixel);
    RefreshCanvas();
    FocusCommandLine();
}

//视图向下平移固定像素
void CCADDlg::OnBnClickedMdown() {
    m_transform.Pan(0, kPanStepPixel);
    RefreshCanvas();
    FocusCommandLine();
}

//视图向左平移固定像素
void CCADDlg::OnBnClickedMl() {
    m_transform.Pan(-kPanStepPixel, 0);
    RefreshCanvas();
    FocusCommandLine();
}

//视图向右平移固定像素
void CCADDlg::OnBnClickedMr() {
    m_transform.Pan(kPanStepPixel, 0);
    RefreshCanvas();
    FocusCommandLine();
}

//打开DXF文件并加载到当前画布
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
    UpdateFileInfoDisplay();
    RefreshCanvas();
    FocusCommandLine();
}

//新建图纸并保存到用户选择的路径
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
    UpdateFileInfoDisplay();
    RefreshCanvas();
    FocusCommandLine();
}

//保存当前图纸
void CCADDlg::OnBnClickedSave() {
    if (m_currentFilePath.IsEmpty()) {
        OnBnClickedNew2();
        return;
    }

    SaveToCurrentPath();
    FocusCommandLine();
}

//另存当前图纸
void CCADDlg::OnBnClickedSaveAs() {
    SaveAsWithDialog();
    FocusCommandLine();
}

//撤销一步操作
void CCADDlg::OnBnClickedUndo() {
    m_shapeMgr.Undo();
    RefreshCanvas();
    FocusCommandLine();
}

//重做一步操作
void CCADDlg::OnBnClickedRedo() {
    m_shapeMgr.Redo();
    RefreshCanvas();
    FocusCommandLine();
}

//激活整线擦除命令
void CCADDlg::OnBnClickedDelLine() {
    ActivateCommand(CADCommandType::ERASER);
}

//激活线段删除命令
void CCADDlg::OnBnClickedDelSegment() {
    ActivateCommand(CADCommandType::DELETE_SEGMENT);
}

//激活插入节点命令
void CCADDlg::OnBnClickedInsertNode() {
    ActivateCommand(CADCommandType::INSERT_NODE);
}

//给选中线条应用颜色，或更新填充预览颜色
void CCADDlg::ApplyColorToSelectedLines(COLORREF color) {
    if (m_bHatchCommandActive) {
        m_hatchColor = color;
        RefreshCanvas();
        FocusCommandLine();
        return;
    }

    std::vector<std::shared_ptr<CLine>> selected;
    for (const auto& shape : m_shapeMgr.GetShapes()) {
        if (shape->IsSelected()) {
            selected.push_back(shape);
        }
    }

    if (selected.empty()) {
        FocusCommandLine();
        return;
    }

    m_shapeMgr.ExecuteCommand(std::make_unique<CChangeLineColorCommand>(&m_shapeMgr, std::move(selected), color));
    RefreshCanvas();
    FocusCommandLine();
}

//把选中对象设置为白色white
void CCADDlg::OnBnClickedColorWhite() { ApplyColorToSelectedLines(kCadColorWhite); }

//把选中对象设置为红色red
void CCADDlg::OnBnClickedColorRed() { ApplyColorToSelectedLines(kCadColorRed); }

//把选中对象设置为黄色yellow
void CCADDlg::OnBnClickedColorYellow() { ApplyColorToSelectedLines(kCadColorYellow); }

//把选中对象设置为绿色green
void CCADDlg::OnBnClickedColorGreen() { ApplyColorToSelectedLines(kCadColorGreen); }

//把选中对象设置为青色cyan
void CCADDlg::OnBnClickedColorCyan() { ApplyColorToSelectedLines(kCadColorCyan); }

//把选中对象设置为蓝色blue
void CCADDlg::OnBnClickedColorBlue() { ApplyColorToSelectedLines(kCadColorBlue); }

//把选中对象设置为洋红色magenta
void CCADDlg::OnBnClickedColorMagenta() { ApplyColorToSelectedLines(kCadColorMagenta); }

//弹出about窗口
void CCADDlg::OnBnClickedAboutIcon() {
    CDialogEx aboutDlg(IDD_ABOUTBOX, this);
    aboutDlg.DoModal();
    FocusCommandLine();
}

//退出程序前检查是否存在未保存修改，弹出窗口供用户选择
void CCADDlg::OnCancel() {
    if (!m_shapeMgr.HasUnsavedChanges()) {
        CDialogEx::OnCancel();
        return;
    }

    const int choice = AfxMessageBox(
        _T("当前有修改尚未保存，是否保存后再退出？"),
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (choice == IDCANCEL) {
        FocusCommandLine();
        return;
    }

    if (choice == IDYES) {
        const bool saved = m_currentFilePath.IsEmpty() ? SaveAsWithDialog() : SaveToCurrentPath();
        if (!saved) {
            FocusCommandLine();
            return;
        }
    }

    CDialogEx::OnCancel();
}