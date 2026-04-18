/*	File: D:\WACKER\tdll\clipbrd.c (Created: 24-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "assert.h"
#include "session.h"
#include "tdll.h"
#include "cloop.h"
#include "ut_text.h"
#include "mc.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CopyBufferToClipBoard
 *
 * DESCRIPTION:
 *	Function to copy text to clipboard
 *
 * ARGUMENTS:
 *	HWND	hwnd	- window that will own clipboard.
 *	DWORD	dwCnt	- size of buffer
 *	void   *pvBuf	- pointer to buffer
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL CopyBufferToClipBoard(const HWND hwnd, const DWORD dwCnt, const void *pvBuf)
	{
	HGLOBAL hMem;
	void *pvMem;
	WCHAR *pszTemp;

	if (pvBuf == 0 || dwCnt == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (!OpenClipboard(hwnd))
		return FALSE;

	hMem = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
			dwCnt + sizeof(WCHAR));

	if (hMem == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	pvMem = GlobalLock(hMem);
    if (dwCnt)
	    MemCopy(pvMem, pvBuf, dwCnt);
	pszTemp = pvMem;
	pszTemp[dwCnt / sizeof(WCHAR)] = L'\0';
	GlobalUnlock(hMem);

	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PasteFromClipboardToHost
 *
 * DESCRIPTION:
 *  This function copies text from the clipboard to host.
 *
 * ARGUMENTS:
 *  HWND hwnd 			- window handle
 *  HSESSION hSession 	- the session handle
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL PasteFromClipboardToHost(const HWND hwnd, const HSESSION hSession)
	{
	HANDLE	hMem;
	LPWSTR  lptStr;

	// Clipboard had to have been opened first...
	//
	if (!OpenClipboard(hwnd))
		return FALSE;

	// Check to see if the clipboard format is available.
	//
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) == FALSE)
		return FALSE;

	// Do we need to enumerate clipboard formats?

	if ((hMem = GetClipboardData(CF_UNICODETEXT)))
		{
		// hMem is owned by the clipboard, so we must not free it 
		// or leave it locked!
		//
		if ((lptStr = GlobalLock(hMem)))
			{
			CLoopSend(sessQueryCLoopHdl(hSession),
					lptStr,
					(size_t)(StrCharGetByteCount(lptStr)),
					0);

			GlobalUnlock(hMem);
			}
		}

	CloseClipboard();
	return TRUE;
	}
