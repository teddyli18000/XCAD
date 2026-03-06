#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

#include <cmath>

// 渲染模块 / rendering module

namespace {
const COLORREF kCadColorWhite = RGB(255, 255, 255);
const COLORREF kCadColorRed = RGB(255, 0, 0);
const COLORREF kCadColorYellow = RGB(255, 255, 0);
const COLORREF kCadColorGreen = RGB(0, 255, 0);
const COLORREF kCadColorCyan = RGB(0, 255, 255);
const COLORREF kCadColorBlue = RGB(0, 0, 255);
const COLORREF kCadColorMagenta = RGB(255, 0, 255);

const COLORREF kCadColorDarkPanel = RGB(33, 33, 33);
const COLORREF kCadColorFrameDark = RGB(30, 30, 30);
const COLORREF kCadColorFrameLight = RGB(220, 220, 220);
const COLORREF kCadColorSquareBorder = RGB(20, 20, 20);

const COLORREF kCadColorAboutPen = RGB(0, 85, 170);
const COLORREF kCadColorAboutFill = RGB(0, 122, 204);

const int kPaletteInnerMargin = 2;
const int kAboutIconInnerMargin = 1;
const int kAboutFontHeight = 11;

const COLORREF kCadColorRulerBg = RGB(50, 50, 50);
const COLORREF kCadColorRulerLine = RGB(150, 150, 150);
const COLORREF kCadColorRulerText = RGB(220, 220, 220);
const int kCadRulerThickness = 20;

//根据按钮 ID 取对应调色板颜色
bool TryGetPaletteColor(int ctrlId, COLORREF& color) {
    switch (ctrlId) {
    case IDC_COLOR_WHITE: color = kCadColorWhite; return true;
    case IDC_COLOR_RED: color = kCadColorRed; return true;
    case IDC_COLOR_YELLOW: color = kCadColorYellow; return true;
    case IDC_COLOR_GREEN: color = kCadColorGreen; return true;
    case IDC_COLOR_CYAN: color = kCadColorCyan; return true;
    case IDC_COLOR_BLUE: color = kCadColorBlue; return true;
    case IDC_COLOR_MAGENTA: color = kCadColorMagenta; return true;
    default: return false;
    }
}
}

// 绘制按钮（颜色按钮与关于图标按钮）
void CCADDlg::DrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    COLORREF paletteColor = kCadColorFrameDark;
    if (TryGetPaletteColor(nIDCtl, paletteColor)) {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);

        CRect rc = lpDrawItemStruct->rcItem;
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));

        dc.Draw3dRect(&rc, kCadColorFrameDark, kCadColorFrameLight);

        CRect square = rc;
        square.DeflateRect(kPaletteInnerMargin, kPaletteInnerMargin);
        CBrush brush(paletteColor);
        CPen pen(PS_SOLID, 1, kCadColorSquareBorder);
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.Rectangle(&square);
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);

        if ((lpDrawItemStruct->itemState & ODS_FOCUS) != 0) {
            dc.DrawFocusRect(&rc);
        }

        dc.Detach();
        return;
    }

    if (nIDCtl == IDC_ABOUT_ICON) {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);

        CRect rc = lpDrawItemStruct->rcItem;
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));

        CRect circleRc = rc;
        circleRc.DeflateRect(kAboutIconInnerMargin, kAboutIconInnerMargin);

        CPen pen(PS_SOLID, 1, kCadColorAboutPen);
        CBrush brush(kCadColorAboutFill);
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.Ellipse(&circleRc);
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);

        int oldBkMode = dc.SetBkMode(TRANSPARENT);
        COLORREF oldTextColor = dc.SetTextColor(kCadColorWhite);

        CFont font;
        font.CreateFont(kAboutFontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
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

// 功能：统一绘制 CAD 画布（双缓冲），减少闪烁
// 交互步骤（OnPaint）：
// 1) 先在内存 DC 里绘制背景和模型（draw to memory DC）
// 2) 再一次性拷贝到屏幕（blit to screen）
// 3) 这样界面更稳定，拖动和预览时不会明显闪烁
void CCADDlg::OnPaint() {
    CPaintDC dc(this);

    CRect rect = m_transform.GetScreenRect();
    if (rect.IsRectEmpty()) return;

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBitmap);

    memDC.FillSolidRect(0, 0, rect.Width(), rect.Height(), kCadColorDarkPanel);

    DrawModel(&memDC);
    DrawPreview(&memDC);
    DrawSelection(&memDC);
    DrawRulers(&memDC);
    DrawCursor(&memDC);

    dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void CCADDlg::DrawRulers(CDC* pDC) {
    if (!pDC) return;

    CRect rect = m_transform.GetScreenRect();
    const int width = rect.Width();
    const int height = rect.Height();
    if (width <= kCadRulerThickness || height <= kCadRulerThickness) return;

    CRect topRuler(0, 0, width, kCadRulerThickness);
    CRect rightRuler(width - kCadRulerThickness, 0, width, height);
    pDC->FillSolidRect(&topRuler, kCadColorRulerBg);
    pDC->FillSolidRect(&rightRuler, kCadColorRulerBg);

    CPoint screenOrigin = m_transform.WorldToScreen(Point2D(0.0, 0.0));
    CPoint screenUnit = m_transform.WorldToScreen(Point2D(1.0, 0.0));
    const double pixelPerWorld = std::fabs(static_cast<double>(screenUnit.x - screenOrigin.x));
    if (pixelPerWorld < 1e-9) return;

    double majorStep = 1.0;
    while (majorStep * pixelPerWorld < 80.0) majorStep *= 2.0;
    while (majorStep * pixelPerWorld > 160.0) majorStep /= 2.0;
    const double minorStep = majorStep / 5.0;

    CPen tickPen(PS_SOLID, 1, kCadColorRulerLine);
    CPen* oldPen = pDC->SelectObject(&tickPen);
    int oldBkMode = pDC->SetBkMode(TRANSPARENT);
    COLORREF oldTextColor = pDC->SetTextColor(kCadColorRulerText);

    const Point2D topLeftW = m_transform.ScreenToWorld(CPoint(0, 0));
    const Point2D topRightW = m_transform.ScreenToWorld(CPoint(width, 0));
    double xMin = topLeftW.x;
    double xMax = topRightW.x;
    if (xMin > xMax) {
        const double tmp = xMin;
        xMin = xMax;
        xMax = tmp;
    }

    const Point2D topW = m_transform.ScreenToWorld(CPoint(0, 0));
    const Point2D bottomW = m_transform.ScreenToWorld(CPoint(0, height));
    double yMin = bottomW.y;
    double yMax = topW.y;
    if (yMin > yMax) {
        const double tmp = yMin;
        yMin = yMax;
        yMax = tmp;
    }

    if (minorStep > 0.0) {
        for (double x = std::floor(xMin / minorStep) * minorStep; x <= xMax; x += minorStep) {
            CPoint tick = m_transform.WorldToScreen(Point2D(x, 0.0));
            if (tick.x < 0 || tick.x >= width - kCadRulerThickness) continue;
            pDC->MoveTo(tick.x, kCadRulerThickness - 4);
            pDC->LineTo(tick.x, kCadRulerThickness);
        }

        for (double y = std::floor(yMin / minorStep) * minorStep; y <= yMax; y += minorStep) {
            CPoint tick = m_transform.WorldToScreen(Point2D(0.0, y));
            if (tick.y < kCadRulerThickness || tick.y >= height) continue;
            pDC->MoveTo(width - 4, tick.y);
            pDC->LineTo(width, tick.y);
        }
    }

    for (double x = std::floor(xMin / majorStep) * majorStep; x <= xMax; x += majorStep) {
        CPoint tick = m_transform.WorldToScreen(Point2D(x, 0.0));
        if (tick.x < 0 || tick.x >= width - kCadRulerThickness) continue;
        pDC->MoveTo(tick.x, kCadRulerThickness - 10);
        pDC->LineTo(tick.x, kCadRulerThickness);

        CString label;
        label.Format(_T("%.2f"), x);
        pDC->TextOut(tick.x + 2, 2, label);
    }

    for (double y = std::floor(yMin / majorStep) * majorStep; y <= yMax; y += majorStep) {
        CPoint tick = m_transform.WorldToScreen(Point2D(0.0, y));
        if (tick.y < kCadRulerThickness || tick.y >= height) continue;
        pDC->MoveTo(width - 10, tick.y);
        pDC->LineTo(width, tick.y);
    }

    pDC->MoveTo(0, kCadRulerThickness - 1);
    pDC->LineTo(width, kCadRulerThickness - 1);
    pDC->MoveTo(width - kCadRulerThickness, 0);
    pDC->LineTo(width - kCadRulerThickness, height);

    pDC->SetTextColor(oldTextColor);
    pDC->SetBkMode(oldBkMode);
    pDC->SelectObject(oldPen);
}

//窗口尺寸变化时，同步更新绘图区矩形
void CCADDlg::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);

    if (GetDlgItem(IDC_DRAW_AREA)) {
        CRect rect;
        GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&rect);
        ScreenToClient(&rect);
        m_transform.SetScreenRect(rect);
        RefreshCanvas();
    }

    UpdateFileInfoLayout();
}

