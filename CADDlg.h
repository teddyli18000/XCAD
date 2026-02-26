
#pragma once

#include "CLine.h"
#include "CShapeManager.h"
#include "CViewTransform.h"

#include <memory>

enum class CADMode {
    MODE_NONE,
    MODE_DRAW,
    MODE_SELECT
};


class CCADDlg : public CDialogEx
{
public:
    CCADDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;
    CString m_currentFilePath;

    CADMode m_currentMode;
    bool m_bIsDrawing;
    bool m_bIsPanning;
    bool m_bShowPoints;
    bool m_bLineCommandActive;
    CPoint m_lastMousePt;

    std::shared_ptr<CLine> m_pCurrentLine;
    CShapeManager m_shapeMgr;
    CViewTransform m_transform;

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnBnClickedDraw();
	afx_msg void OnBnClickedSel();
	afx_msg void OnBnClickedViewPoint();
	afx_msg void OnBnClickedHidePoint();
	afx_msg void OnBnClickedZoomin();
	afx_msg void OnBnClickedZoomout();
	afx_msg void OnBnClickedZoomdef();
	afx_msg void OnBnClickedMup();
	afx_msg void OnBnClickedMdown();
	afx_msg void OnBnClickedMl();
	afx_msg void OnBnClickedMr();
	afx_msg void OnBnClickedOpen();
	afx_msg void OnBnClickedNew2();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedSaveAs();
	afx_msg void OnBnClickedUndo();
	afx_msg void OnBnClickedRedo();

	void ProcessCommandLine(const CString& cmd);
	void CancelCurrentDrawing();
	void FinishCurrentDrawing(bool keepCommandActive);
	void FocusCommandLine();
	void RefreshCanvas();
	bool SaveToCurrentPath();
	bool SaveAsWithDialog();

	DECLARE_MESSAGE_MAP()
};
