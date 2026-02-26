
// CAD.h: PROJECT_NAME Ó¦ÓÃ³ÌÐòµÄÖ÷Í·ÎÄ¼þ
//

#pragma once

#ifndef __AFXWIN_H__
	#error "ÔÚ°üº¬´ËÎÄ¼þÖ®Ç°°üº¬ 'pch.h' ÒÔÉú³É PCH"
#endif

#include "resource.h"		// Ö÷·ûºÅ


// CCADApp:
// ÓÐ¹Ø´ËÀàµÄÊµÏÖ£¬Çë²ÎÔÄ CAD.cpp
//

class CCADApp : public CWinApp
{
public:
	CCADApp();

// ÖØÐ´
public:
	virtual BOOL InitInstance();

// ÊµÏÖ

	DECLARE_MESSAGE_MAP()
};

extern CCADApp theApp;
