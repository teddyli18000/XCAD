
#pragma once

#include "CLine.h"
#include "CShapeManager.h"
#include "CViewTransform.h"

#include <memory>
#include <vector>

enum class CADMode {
    MODE_NONE,
    MODE_DRAW,
    MODE_SELECT
};

enum class CADCommandType {
    NONE,
    LINE,
    CIRCLE,
    RECTANGLE,
    ARC,
    HATCH,
    ERASER,
    DELETE_NODE
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
    bool m_bCircleCommandActive;
    bool m_bCircleCenterPicked;
    bool m_bRectangleCommandActive;
    bool m_bRectangleFirstPicked;
    bool m_bArcCommandActive;
    bool m_bHatchCommandActive;
    bool m_bEraserCommandActive;
    bool m_bDeleteNodeCommandActive;
    bool m_bHatchPreviewVisible;
    bool m_bIsSelectingBox;
    bool m_bIsErasing;
    bool m_bEraserCursorVisible;
    int m_arcPointCount;
    int m_eraserRadius;
    CPoint m_lastMousePt;
    CPoint m_selectBoxStart;
    CPoint m_selectBoxEnd;
    CPoint m_eraserCursor;
    CPoint m_hatchPreviewPoint;
    Point2D m_circleCenter;
    Point2D m_circlePreviewPoint;
    Point2D m_rectFirstPoint;
    Point2D m_rectPreviewPoint;
    Point2D m_arcStartPoint;
    Point2D m_arcSecondPoint;
    Point2D m_arcPreviewPoint;
    COLORREF m_hatchColor;

    std::shared_ptr<CLine> m_pCurrentLine;
    std::vector<std::unique_ptr<CBitmap>> m_colorButtonBitmaps;
    CShapeManager m_shapeMgr;
    CViewTransform m_transform;

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
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
	afx_msg void OnBnClickedCircle();
	afx_msg void OnBnClickedRectangle();
	afx_msg void OnBnClickedArc();
	afx_msg void OnBnClickedHatch();
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
	afx_msg void OnBnClickedAboutIcon();
	afx_msg void OnBnClickedDelLine();
	afx_msg void OnBnClickedDelPoint();
	afx_msg void OnBnClickedColorWhite();
	afx_msg void OnBnClickedColorRed();
	afx_msg void OnBnClickedColorYellow();
	afx_msg void OnBnClickedColorGreen();
	afx_msg void OnBnClickedColorCyan();
	afx_msg void OnBnClickedColorBlue();
	afx_msg void OnBnClickedColorMagenta();

	void ProcessCommandLine(const CString& cmd);
	void CancelCurrentDrawing();
	void CancelActiveCommand();
	void FinishCurrentDrawing(bool keepCommandActive);
	void FocusCommandLine();
	void RefreshCanvas();
	bool SaveToCurrentPath();
	bool SaveAsWithDialog();
	void ActivateCommand(CADCommandType commandType);
	std::shared_ptr<CLine> CreateCirclePolyline(const Point2D& center, double radius, int segments) const;
	std::shared_ptr<CLine> CreateRectanglePolyline(const Point2D& first, const Point2D& second) const;
	std::shared_ptr<CLine> CreateArcPolylineByThreePoints(const Point2D& start, const Point2D& through, const Point2D& end, int segments) const;
	void ClearSelection();
	void ApplySelectionBox();
	void DeleteSelectedLines();
	void ApplyColorToSelectedLines(COLORREF color);
	void EraseAtPoint(const CPoint& localPt);
	void DrawModel(CDC* pDC);
	void DrawPreview(CDC* pDC);
	void DrawHatchPreview(CDC* pDC);
	void DrawSelection(CDC* pDC);
	void DrawCursor(CDC* pDC);
	bool HandleLineToolLButtonDown(const Point2D& worldPt);
	bool HandleLineToolMouseMove(const Point2D& worldPt);
	bool HandleCircleToolLButtonDown(const Point2D& worldPt);
	bool HandleCircleToolMouseMove(const Point2D& worldPt);
	bool HandleRectToolLButtonDown(const Point2D& worldPt);
	bool HandleRectToolMouseMove(const Point2D& worldPt);
	bool HandleArcToolLButtonDown(const Point2D& worldPt);
	bool HandleArcToolMouseMove(const Point2D& worldPt);
	bool HandleHatchToolLButtonDown(const CPoint& localPt);
	bool HandleHatchToolMouseMove(const CPoint& localPt, bool inCanvas);
	bool HandleSelectionToolLButtonDown(const CPoint& localPt);
	bool HandleSelectionToolMouseMove(const CPoint& localPt);
	bool HandleSelectionToolLButtonUp(const CPoint& localPt);
	bool HandleEraserToolLButtonDown(const CPoint& localPt);
	bool HandleEraserToolMouseMove(UINT nFlags, const CPoint& localPt, bool inCanvas);
	bool HandleEraserToolLButtonUp();
	void UpdateModeButtonHighlight();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedDrawArea();
};
