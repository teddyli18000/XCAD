#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // ´Ó Windows Í·ÖÐÅÅ³ý¼«ÉÙÊ¹ÓÃµÄ×ÊÁÏ
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // Ä³Ð© CString ¹¹Ôìº¯Êý½«ÊÇÏÔÊ½µÄ

// ¹Ø±Õ MFC µÄÒ»Ð©³£¼ûÇÒ¾­³£¿É·ÅÐÄºöÂÔµÄÒþ²Ø¾¯¸æÏûÏ¢
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ºËÐÄ×é¼þºÍ±ê×¼×é¼þ
#include <afxext.h>         // MFC À©Õ¹


#include <afxdisp.h>        // MFC ×Ô¶¯»¯Àà



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC ¶Ô Internet Explorer 4 ¹«¹²¿Ø¼þµÄÖ§³Ö
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC ¶Ô Windows ¹«¹²¿Ø¼þµÄÖ§³Ö
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC Ö§³Ö¹¦ÄÜÇøºÍ¿ØÖÆÌõ









#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


