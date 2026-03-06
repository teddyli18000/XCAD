#include "pch.h"
#include "CADDlg.h"

//绘图区静态控件点击响应（迁移至鼠标处理）
//（未接入）
void CCADDlg::OnStnClickedDrawArea()
{
    //绘图区交互由鼠标消息统一处理，所以删除了这里的代码，为防止bug，保留函数名
}
/*move to
* 
CADlg.cpp
ON_WM_LBUTTONUP
ON_WM_LBUTTONDOWN
ON_WM_MOUSEMOVE

CADlgInput.cpp(处理)
CCADDlg::OnLButtonDown(...)
CCADDlg::OnMouseMove(...)
CCADDlg::OnLButtonUp(...)

*/