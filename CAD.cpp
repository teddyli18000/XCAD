
// CAD.cpp: ¶¨ÒåÓ¦ÓÃ³ÌÐòµÄÀàÐÐÎª¡£
//

#include "pch.h"
#include "framework.h"
#include "CAD.h"
#include "CADDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCADApp

BEGIN_MESSAGE_MAP(CCADApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CCADApp ¹¹Ôì

CCADApp::CCADApp()
{
	// Ö§³ÖÖØÐÂÆô¶¯¹ÜÀíÆ÷
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: ÔÚ´Ë´¦Ìí¼Ó¹¹Ôì´úÂë£¬
	// ½«ËùÓÐÖØÒªµÄ³õÊ¼»¯·ÅÖÃÔÚ InitInstance ÖÐ
}


// Î¨Ò»µÄ CCADApp ¶ÔÏó

CCADApp theApp;


// CCADApp ³õÊ¼»¯

BOOL CCADApp::InitInstance()
{
	// Èç¹ûÓ¦ÓÃ³ÌÐò´æÔÚÒÔÏÂÇé¿ö£¬Windows XP ÉÏÐèÒª InitCommonControlsEx()
	// Ê¹ÓÃ ComCtl32.dll °æ±¾ 6 »ò¸ü¸ß°æ±¾À´ÆôÓÃ¿ÉÊÓ»¯·½Ê½£¬
	//ÔòÐèÒª InitCommonControlsEx()¡£  ·ñÔò£¬½«ÎÞ·¨´´½¨´°¿Ú¡£
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ½«ËüÉèÖÃÎª°üÀ¨ËùÓÐÒªÔÚÓ¦ÓÃ³ÌÐòÖÐÊ¹ÓÃµÄ
	// ¹«¹²¿Ø¼þÀà¡£
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// ´´½¨ shell ¹ÜÀíÆ÷£¬ÒÔ·À¶Ô»°¿ò°üº¬
	// ÈÎºÎ shell Ê÷ÊÓÍ¼¿Ø¼þ»ò shell ÁÐ±íÊÓÍ¼¿Ø¼þ¡£
	CShellManager *pShellManager = new CShellManager;

	// ¼¤»î¡°Windows Native¡±ÊÓ¾õ¹ÜÀíÆ÷£¬ÒÔ±ãÔÚ MFC ¿Ø¼þÖÐÆôÓÃÖ÷Ìâ
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// ±ê×¼³õÊ¼»¯
	// Èç¹ûÎ´Ê¹ÓÃÕâÐ©¹¦ÄÜ²¢Ï£Íû¼õÐ¡
	// ×îÖÕ¿ÉÖ´ÐÐÎÄ¼þµÄ´óÐ¡£¬ÔòÓ¦ÒÆ³ýÏÂÁÐ
	// ²»ÐèÒªµÄÌØ¶¨³õÊ¼»¯Àý³Ì
	// ¸ü¸ÄÓÃÓÚ´æ´¢ÉèÖÃµÄ×¢²á±íÏî
	// TODO: Ó¦ÊÊµ±ÐÞ¸Ä¸Ã×Ö·û´®£¬
	// ÀýÈçÐÞ¸ÄÎª¹«Ë¾»ò×éÖ¯Ãû
	SetRegistryKey(_T("Ó¦ÓÃ³ÌÐòÏòµ¼Éú³ÉµÄ±¾µØÓ¦ÓÃ³ÌÐò"));

	CCADDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: ÔÚ´Ë·ÅÖÃ´¦ÀíºÎÊ±ÓÃ
		//  ¡°È·¶¨¡±À´¹Ø±Õ¶Ô»°¿òµÄ´úÂë
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: ÔÚ´Ë·ÅÖÃ´¦ÀíºÎÊ±ÓÃ
		//  ¡°È¡Ïû¡±À´¹Ø±Õ¶Ô»°¿òµÄ´úÂë
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "¾¯¸æ: ¶Ô»°¿ò´´½¨Ê§°Ü£¬Ó¦ÓÃ³ÌÐò½«ÒâÍâÖÕÖ¹¡£\n");
		TRACE(traceAppMsg, 0, "¾¯¸æ: Èç¹ûÄúÔÚ¶Ô»°¿òÉÏÊ¹ÓÃ MFC ¿Ø¼þ£¬ÔòÎÞ·¨ #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS¡£\n");
	}

	// É¾³ýÉÏÃæ´´½¨µÄ shell ¹ÜÀíÆ÷¡£
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// ÓÉÓÚ¶Ô»°¿òÒÑ¹Ø±Õ£¬ËùÒÔ½«·µ»Ø FALSE ÒÔ±ãÍË³öÓ¦ÓÃ³ÌÐò£¬
	//  ¶ø²»ÊÇÆô¶¯Ó¦ÓÃ³ÌÐòµÄÏûÏ¢±Ã¡£
	return FALSE;
}

