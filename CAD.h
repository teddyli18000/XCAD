// CAD app header / 应用头文件

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before this file / 先包含 pch.h"
#endif

#include "resource.h"		// resources / 资源


// CCADApp / 应用类

class CCADApp : public CWinApp
{
public:
	CCADApp();

// override / 重写
public:
	virtual BOOL InitInstance();

// impl / 实现

	DECLARE_MESSAGE_MAP()
};

extern CCADApp theApp;
