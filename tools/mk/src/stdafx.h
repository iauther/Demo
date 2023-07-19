// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER					0x0600
#define _WIN32_WINNT		0x0600
#define _WIN32_IE				0x0700
#define _RICHEDIT_VER		0x0300

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;
#define _WTL_N_CSTRING

#include <atlmisc.h>
#include <atlcrack.h>
#include <atlframe.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atltheme.h>
#include <atlddx.h>
#include <atlsplit.h>
