#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

// 命令解析模块 / command line module
// `ProcessCommandLine(cmd)`
// - cmd: 原始命令字符串 / raw command text
// - 自动 trim + upper + 去空格 / normalize command text

namespace {
const SHORT kKeyDownMask = static_cast<SHORT>(0x8000);
}

// 功能：解析命令行文本并分发到对应命令处理。
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
    } else if (normalized == _T("T") || normalized == _T("TEXT")) {
        ActivateCommand(CADCommandType::TEXT);
    } else if (normalized == _T("A") || normalized == _T("ARC")) {
        ActivateCommand(CADCommandType::ARC);
    } else if (normalized == _T("H") || normalized == _T("HATCH")) {
        ActivateCommand(CADCommandType::HATCH);
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
        // pan by middle button / 中键平移
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

// 功能：统一处理快捷键与命令行回车提交。
// 交互步骤（keyboard input）：
// 1) 先拦截 Ctrl+Z / Ctrl+Y / ESC / Delete。
// 2) 再处理命令行输入框里的 Enter 提交。
// 3) 最后回落到基类消息流程，保持 MFC 默认行为。
BOOL CCADDlg::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_KEYDOWN) {
        if (m_bTextInputActive) {
            if (pMsg->wParam == VK_RETURN) {
                CommitTextInput(true);
                RefreshCanvas();
                FocusCommandLine();
                return TRUE;
            }
            if (pMsg->wParam == VK_ESCAPE) {
                CommitTextInput(false);
                RefreshCanvas();
                FocusCommandLine();
                return TRUE;
            }
        }

        const bool ctrlDown = (GetKeyState(VK_CONTROL) & kKeyDownMask) != 0;

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
            m_currentMode == CADMode::MODE_SELECT && !m_bEraserCommandActive && !m_bDeleteSegmentCommandActive && !m_bInsertNodeCommandActive && !m_bHatchCommandActive) {
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
        TCHAR className[32] = { 0 };
        if (::GetClassName(pMsg->hwnd, className, _countof(className)) > 0 && _tcsicmp(className, _T("Button")) == 0) {
            return handled;
        }
        FocusCommandLine();
    }

    return handled;
}
