/*	File: D:\WACKER\tdll\misc.h (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

#if !defined(INCL_MISC)
#define INCL_MISC

BOOL 	mscCenterWindowOnWindow(const HWND hwndChild, const HWND hwndParent);

LPWSTR 	mscStripPath	(LPWSTR pszStr);
LPWSTR 	mscStripName	(LPWSTR pszStr);
LPWSTR 	mscStripExt		(LPWSTR pszStr);
LPWSTR	mscModifyToFit	(HWND hwnd, LPWSTR pszStr);

int 	mscCreatePath(const WCHAR *pszPath);
int 	mscIsDirectory(LPCWSTR pszName);
int 	mscAskWizardQuestionAgain(void);
void	mscUpdateRegistryValue(void);

HICON	extLoadIcon(LPCSTR);
#endif
