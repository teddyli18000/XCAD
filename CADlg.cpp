#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"
#include "CADlgGeometryUtils.h"
#include "afxdialogex.h"

#include <afxdlgs.h>
#include <cmath>
#include <memory>
#include <vector>

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

void CCADDlg::DrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    if (nIDCtl == IDC_ABOUT_ICON) {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);

        CRect rc = lpDrawItemStruct->rcItem;
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));

        CRect circleRc = rc;
        circleRc.DeflateRect(1, 1);

        CPen pen(PS_SOLID, 1, RGB(0, 85, 170));
        CBrush brush(RGB(0, 122, 204));
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.Ellipse(&circleRc);
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);

        int oldBkMode = dc.SetBkMode(TRANSPARENT);
        COLORREF oldTextColor = dc.SetTextColor(RGB(255, 255, 255));

        CFont font;
        font.CreateFont(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
        CFont* oldFont = dc.SelectObject(&font);

        dc.DrawText(_T("?"), &circleRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        dc.SelectObject(oldFont);
        dc.SetTextColor(oldTextColor);
        dc.SetBkMode(oldBkMode);

        if ((lpDrawItemStruct->itemState & ODS_FOCUS) != 0) {
            dc.DrawFocusRect(&rc);
        }

        dc.Detach();
        return;
    }

    UNREFERENCED_PARAMETER(lpDrawItemStruct);
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

    if (m_bCircleCommandActive && m_bCircleCenterPicked) {
        const double dx = m_circlePreviewPoint.x - m_circleCenter.x;
        const double dy = m_circlePreviewPoint.y - m_circleCenter.y;
        const double radius = std::sqrt(dx * dx + dy * dy);

        CPoint centerPt = m_transform.WorldToScreen(m_circleCenter);
        CPoint previewPt = m_transform.WorldToScreen(m_circlePreviewPoint);

        CPen dashPen(PS_DASH, 1, RGB(200, 200, 200));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);

        memDC.MoveTo(centerPt);
        memDC.LineTo(previewPt);

        CPoint radiusPt = m_transform.WorldToScreen(Point2D(m_circleCenter.x + radius, m_circleCenter.y));
        int r = radiusPt.x - centerPt.x;
        if (r < 0) r = -r;
        if (r > 0) {
            memDC.Ellipse(centerPt.x - r, centerPt.y - r, centerPt.x + r, centerPt.y + r);
        }

        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_bRectangleCommandActive && m_bRectangleFirstPicked) {
        CPoint p1 = m_transform.WorldToScreen(m_rectFirstPoint);
        CPoint p3 = m_transform.WorldToScreen(m_rectPreviewPoint);
        CPoint p2(p3.x, p1.y);
        CPoint p4(p1.x, p3.y);

        CPen dashPen(PS_DASH, 1, RGB(200, 200, 200));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);

        memDC.MoveTo(p1);
        memDC.LineTo(p2);
        memDC.LineTo(p3);
        memDC.LineTo(p4);
        memDC.LineTo(p1);

        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_bArcCommandActive && m_arcPointCount > 0) {
        CPen dashPen(PS_DASH, 1, RGB(200, 200, 200));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);

        if (m_arcPointCount == 1) {
            CPoint ps = m_transform.WorldToScreen(m_arcStartPoint);
            CPoint pp = m_transform.WorldToScreen(m_arcPreviewPoint);
            memDC.MoveTo(ps);
            memDC.LineTo(pp);
        } else if (m_arcPointCount == 2) {
            std::shared_ptr<CLine> previewArc = CreateArcPolylineByThreePoints(m_arcStartPoint, m_arcSecondPoint, m_arcPreviewPoint, 72);
            const auto& pts = previewArc->GetPoints();
            if (!pts.empty()) {
                CPoint startPt = m_transform.WorldToScreen(pts[0]);
                memDC.MoveTo(startPt);
                for (size_t i = 1; i < pts.size(); ++i) {
                    memDC.LineTo(m_transform.WorldToScreen(pts[i]));
                }
            }
        }

        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox) {
        CRect box = cad::dlg::NormalizeRect(m_selectBoxStart, m_selectBoxEnd);
        CPen dashPen(PS_DASH, 1, RGB(255, 255, 255));
        CPen* oldPen = memDC.SelectObject(&dashPen);
        int oldBkMode = memDC.SetBkMode(TRANSPARENT);
        memDC.Rectangle(&box);
        memDC.SetBkMode(oldBkMode);
        memDC.SelectObject(oldPen);
    }

    if (m_bEraserCommandActive && m_bEraserCursorVisible) {
        CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
        CPen* oldPen = memDC.SelectObject(&pen);
        CBrush* oldBrush = static_cast<CBrush*>(memDC.SelectStockObject(NULL_BRUSH));
        memDC.Ellipse(m_eraserCursor.x - m_eraserRadius, m_eraserCursor.y - m_eraserRadius,
            m_eraserCursor.x + m_eraserRadius, m_eraserCursor.y + m_eraserRadius);
        memDC.SelectObject(oldBrush);
        memDC.SelectObject(oldPen);
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
    } else if (m_currentMode == CADMode::MODE_DRAW && m_bCircleCommandActive) {
        if (!m_bCircleCenterPicked) {
            m_bCircleCenterPicked = true;
            m_circleCenter = worldPt;
            m_circlePreviewPoint = worldPt;
        } else {
            const double dx = worldPt.x - m_circleCenter.x;
            const double dy = worldPt.y - m_circleCenter.y;
            const double radius = std::sqrt(dx * dx + dy * dy);
            if (radius > 0.0001) {
                m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateCirclePolyline(m_circleCenter, radius, 96)));
            }
            m_bCircleCenterPicked = false;
            m_bCircleCommandActive = false;
        }
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_DRAW && m_bRectangleCommandActive) {
        if (!m_bRectangleFirstPicked) {
            m_bRectangleFirstPicked = true;
            m_rectFirstPoint = worldPt;
            m_rectPreviewPoint = worldPt;
        } else {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, CreateRectanglePolyline(m_rectFirstPoint, worldPt)));
            m_bRectangleFirstPicked = false;
            m_bRectangleCommandActive = false;
        }
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_DRAW && m_bArcCommandActive) {
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
        RefreshCanvas();
    } else if (m_currentMode == CADMode::MODE_SELECT) {
        ClearSelection();
        if (m_bEraserCommandActive) {
            m_bIsErasing = true;
            m_eraserCursor = localPt;
            m_bEraserCursorVisible = true;
            EraseAtPoint(localPt);
        } else {
            m_bIsSelectingBox = true;
            m_selectBoxStart = localPt;
            m_selectBoxEnd = localPt;
        }
        SetCapture();
        RefreshCanvas();
    }

    FocusCommandLine();
}

void CCADDlg::OnMouseMove(UINT nFlags, CPoint point) {
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
        return;
    }

    if (m_bCircleCommandActive && m_bCircleCenterPicked) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        m_circlePreviewPoint = m_transform.ScreenToWorld(localPt);
        RefreshCanvas();
        return;
    }

    if (m_bRectangleCommandActive && m_bRectangleFirstPicked) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        m_rectPreviewPoint = m_transform.ScreenToWorld(localPt);
        RefreshCanvas();
        return;
    }

    if (m_bArcCommandActive && m_arcPointCount > 0) {
        CRect rect = m_transform.GetScreenRect();
        CPoint localPt(point.x - rect.left, point.y - rect.top);
        m_arcPreviewPoint = m_transform.ScreenToWorld(localPt);
        RefreshCanvas();
        return;
    }

    CRect rect = m_transform.GetScreenRect();
    bool inCanvas = rect.PtInRect(point);
    CPoint localPt(point.x - rect.left, point.y - rect.top);

    if (m_bEraserCommandActive) {
        m_bEraserCursorVisible = inCanvas;
        if (inCanvas) {
            m_eraserCursor = localPt;
            if (m_bIsErasing && (nFlags & MK_LBUTTON)) {
                EraseAtPoint(localPt);
            }
        }
        RefreshCanvas();
        return;
    }

    if (m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox) {
        m_selectBoxEnd = localPt;
        RefreshCanvas();
    }
}

void CCADDlg::OnLButtonUp(UINT nFlags, CPoint point) {
    CRect rect = m_transform.GetScreenRect();
    CPoint localPt(point.x - rect.left, point.y - rect.top);

    if (m_currentMode == CADMode::MODE_SELECT && m_bIsSelectingBox) {
        m_selectBoxEnd = localPt;
        ApplySelectionBox();
        m_bIsSelectingBox = false;
        if (GetCapture() == this) {
            ReleaseCapture();
        }
        RefreshCanvas();
    }

    if (m_bEraserCommandActive && m_bIsErasing) {
        m_bIsErasing = false;
        if (GetCapture() == this) {
            ReleaseCapture();
        }
    }

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
    m_bCircleCommandActive = false;
    m_bCircleCenterPicked = false;
    m_bRectangleCommandActive = false;
    m_bRectangleFirstPicked = false;
    m_bArcCommandActive = false;
    m_arcPointCount = 0;
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::OnRButtonDown(UINT nFlags, CPoint point) {
    UNREFERENCED_PARAMETER(nFlags);
    UNREFERENCED_PARAMETER(point);
    if (m_bIsDrawing && m_bLineCommandActive) {
        FinishCurrentDrawing(false);
    } else {
        CancelActiveCommand();
    }
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
    if (GetCapture() == this) {
        ReleaseCapture();
    }
    ClearSelection();
    RefreshCanvas();
    FocusCommandLine();
}

void CCADDlg::CancelActiveCommand() {
    if (m_bIsDrawing && m_bLineCommandActive && m_pCurrentLine) {
        auto& pts = const_cast<std::vector<Point2D>&>(m_pCurrentLine->GetPoints());
        if (pts.size() > 1) pts.pop_back();
        if (pts.size() >= 2) {
            m_shapeMgr.ExecuteCommand(std::make_unique<CAddLineCommand>(&m_shapeMgr, m_pCurrentLine));
        }
    }

    m_bIsDrawing = false;
    m_pCurrentLine.reset();
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

    if (GetCapture() == this) {
        ReleaseCapture();
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

void CCADDlg::ProcessCommandLine(const CString& cmd) {
    CString normalized = cmd;
    normalized.Trim();
    normalized.MakeUpper();
    normalized.Replace(_T(" "), _T(""));

    if (normalized.IsEmpty()) return;

    if (normalized == _T("L") || normalized == _T("LINE") || normalized == _T("PL") || normalized == _T("PLINE")) {
        ActivateCommand(CADCommandType::LINE);
    } else if (normalized == _T("C") || normalized == _T("CIRCLE")) {
        ActivateCommand(CADCommandType::CIRCLE);
    } else if (normalized == _T("REC") || normalized == _T("RECT") || normalized == _T("RECTANGLE") || normalized == _T("RECTANG")) {
        ActivateCommand(CADCommandType::RECTANGLE);
    } else if (normalized == _T("A") || normalized == _T("ARC")) {
        ActivateCommand(CADCommandType::ARC);
    } else if (normalized == _T("E") || normalized == _T("ER") || normalized == _T("ERASE")) {
        ActivateCommand(CADCommandType::ERASER);
    } else if (normalized == _T("ESC") || normalized == _T("CANCEL")) {
        CancelActiveCommand();
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
    } else if (normalized == _T("SAVEAS")) {
        OnBnClickedSaveAs();
    } else if (normalized == _T("SELECT") || normalized == _T("SEL")) {
        OnBnClickedSel();
    } else if (normalized == _T("DRAW")) {
        OnBnClickedDraw();
    } else if (normalized == _T("ZOOM") || normalized == _T("ZI")) {
        OnBnClickedZoomin();
    } else if (normalized == _T("ZO") || normalized == _T("ZOOMOUT")) {
        OnBnClickedZoomout();
    } else if (normalized == _T("ZE") || normalized == _T("ZOOMEXTENTS")) {
        OnBnClickedZoomdef();
    } else if (normalized == _T("PAN") || normalized == _T("P")) {
        // 平移命令进入后用中键拖动/按钮平移
    } else if (normalized == _T("REGEN")) {
        RefreshCanvas();
    } else if (normalized == _T("POINTON") || normalized == _T("SHOWPOINT") || normalized == _T("SHOWPOINTS")) {
        OnBnClickedViewPoint();
    } else if (normalized == _T("POINTOFF") || normalized == _T("HIDEPOINT") || normalized == _T("HIDEPOINTS")) {
        OnBnClickedHidePoint();
    } else if (normalized == _T("CTRLZ")) {
        OnBnClickedUndo();
    } else if (normalized == _T("CTRLY")) {
        OnBnClickedRedo();
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
            CancelActiveCommand();
            return TRUE;
        }

        if ((pMsg->wParam == VK_DELETE || pMsg->wParam == VK_BACK) &&
            m_currentMode == CADMode::MODE_SELECT && !m_bEraserCommandActive) {
            DeleteSelectedLines();
            FocusCommandLine();
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