// CAD app entry / 程序入口

#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// App message map / 消息映射

BEGIN_MESSAGE_MAP(CCADApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// ctor / 构造

CCADApp::CCADApp()
{
	// restart support / 重启支持
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// init in InitInstance / 初始化放到 InitInstance
}


// global app / 全局应用对象

CCADApp theApp;


// app init / 应用初始化

BOOL CCADApp::InitInstance()
{
	// init common controls / 初始化通用控件
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Win95 control classes / Win95 控件类
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// shell manager / Shell 管理器
	CShellManager *pShellManager = new CShellManager;

	// native visual manager / 原生主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// registry key / 注册表键
	SetRegistryKey(_T("CAD Local App"));

	CCADDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// ok closed / 确定关闭
	}
	else if (nResponse == IDCANCEL)
	{
		// cancel closed / 取消关闭
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog create failed.\n");
		TRACE(traceAppMsg, 0, "Warning: check MFC controls in dialogs macro.\n");
	}

	// release shell manager / 释放管理器
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// exit app after dialog / 对话框关闭后退出应用
	return FALSE;
}

