/*	File: D:\WACKER\tdll\termhdl.c (Created: 10-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 7 $
 *	$Date: 4/12/01 3:59p $
 */

#include <windows.h>
#pragma hdrstop

#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "stdtyp.h"
#include <term/res.h>
#include "tdll.h"
#include "globals.h"
#include "mc.h"
#include "assert.h"
#include "session.h"
#include "timers.h"
#include "update.h"
#include <emu/emu.h>
#include <emu/emu.hh>
#include "ut_text.h"
#include "term.h"
#include "term.hh"

typedef struct stTermColorTheme
	{
	int nTheme;
	const WCHAR *pszName;
	int nDefaultText;
	int nDefaultBackground;
	COLORREF acrColors[MAX_EMUCOLORS];
	} STTERMCOLORTHEME;

static const STTERMCOLORTHEME astTermColorThemes[] =
	{
		{
		EMU_COLOR_THEME_SOLARIZED_LIGHT,
		L"Solarized Light",
		VC_BRT_CYAN,
		VC_BRT_WHITE,
			{
			RGB(0x07, 0x36, 0x42), RGB(0xdc, 0x32, 0x2f),
			RGB(0x85, 0x99, 0x00), RGB(0xb5, 0x89, 0x00),
			RGB(0x26, 0x8b, 0xd2), RGB(0xd3, 0x36, 0x82),
			RGB(0x2a, 0xa1, 0x98), RGB(0xee, 0xe8, 0xd5),
			RGB(0x00, 0x2b, 0x36), RGB(0xcb, 0x4b, 0x16),
			RGB(0x58, 0x6e, 0x75), RGB(0x65, 0x7b, 0x83),
			RGB(0x83, 0x94, 0x96), RGB(0x6c, 0x71, 0xc4),
			RGB(0x93, 0xa1, 0xa1), RGB(0xfd, 0xf6, 0xe3)
			}
		},
		{
		EMU_COLOR_THEME_SOLARIZED_DARK,
		L"Solarized Dark",
		VC_BRT_BLUE,
		VC_GRAY,
			{
			RGB(0x07, 0x36, 0x42), RGB(0xdc, 0x32, 0x2f),
			RGB(0x85, 0x99, 0x00), RGB(0xb5, 0x89, 0x00),
			RGB(0x26, 0x8b, 0xd2), RGB(0xd3, 0x36, 0x82),
			RGB(0x2a, 0xa1, 0x98), RGB(0xee, 0xe8, 0xd5),
			RGB(0x00, 0x2b, 0x36), RGB(0xcb, 0x4b, 0x16),
			RGB(0x58, 0x6e, 0x75), RGB(0x65, 0x7b, 0x83),
			RGB(0x83, 0x94, 0x96), RGB(0x6c, 0x71, 0xc4),
			RGB(0x93, 0xa1, 0xa1), RGB(0xfd, 0xf6, 0xe3)
			}
		},
		{
		EMU_COLOR_THEME_SELENIZED_LIGHT,
		L"Selenized Light",
		VC_BRT_CYAN,
		VC_BRT_WHITE,
			{
			RGB(0x53, 0x67, 0x6d), RGB(0xd2, 0x21, 0x2d),
			RGB(0x48, 0x91, 0x00), RGB(0xad, 0x89, 0x00),
			RGB(0x00, 0x72, 0xd4), RGB(0xca, 0x48, 0x98),
			RGB(0x00, 0x9c, 0x8f), RGB(0xec, 0xe3, 0xcc),
			RGB(0x3a, 0x4d, 0x53), RGB(0xc2, 0x5d, 0x1e),
			RGB(0x90, 0x99, 0x95), RGB(0x53, 0x67, 0x6d),
			RGB(0x00, 0x6d, 0xce), RGB(0x87, 0x62, 0xc6),
			RGB(0xd5, 0xcd, 0xb6), RGB(0xfb, 0xf3, 0xdb)
			}
		},
		{
		EMU_COLOR_THEME_SELENIZED_DARK,
		L"Selenized Dark",
		VC_BRT_CYAN,
		VC_BLACK,
			{
			RGB(0x10, 0x3c, 0x48), RGB(0xfa, 0x57, 0x50),
			RGB(0x75, 0xb9, 0x38), RGB(0xdb, 0xb3, 0x2d),
			RGB(0x46, 0x95, 0xf7), RGB(0xf2, 0x75, 0xbe),
			RGB(0x41, 0xc7, 0xb9), RGB(0xad, 0xbc, 0xbc),
			RGB(0x18, 0x49, 0x56), RGB(0xed, 0x86, 0x49),
			RGB(0x84, 0xc7, 0x47), RGB(0xeb, 0xc1, 0x3d),
			RGB(0x58, 0xa3, 0xff), RGB(0xaf, 0x88, 0xeb),
			RGB(0x53, 0xd6, 0xc7), RGB(0xca, 0xd8, 0xd9)
			}
		},
		{
		EMU_COLOR_THEME_SELENIZED_BLACK,
		L"Selenized Black",
		VC_BRT_CYAN,
		VC_BLACK,
			{
			RGB(0x18, 0x18, 0x18), RGB(0xed, 0x4a, 0x46),
			RGB(0x70, 0xb4, 0x33), RGB(0xdb, 0xb3, 0x2d),
			RGB(0x36, 0x8a, 0xeb), RGB(0xeb, 0x6e, 0xb7),
			RGB(0x3f, 0xc5, 0xb7), RGB(0xb9, 0xb9, 0xb9),
			RGB(0x25, 0x25, 0x25), RGB(0xe6, 0x7f, 0x43),
			RGB(0x83, 0xc7, 0x46), RGB(0xef, 0xc5, 0x41),
			RGB(0x4f, 0x9c, 0xfe), RGB(0xa5, 0x80, 0xe2),
			RGB(0x56, 0xd8, 0xc9), RGB(0xde, 0xde, 0xde)
			}
		},
		{
		EMU_COLOR_THEME_SELENIZED_WHITE,
		L"Selenized White",
		VC_BRT_CYAN,
		VC_BRT_WHITE,
			{
			RGB(0x47, 0x47, 0x47), RGB(0xd6, 0x00, 0x0c),
			RGB(0x1d, 0x97, 0x00), RGB(0xc4, 0x97, 0x00),
			RGB(0x00, 0x64, 0xe4), RGB(0xdd, 0x0f, 0x9d),
			RGB(0x00, 0xad, 0x9c), RGB(0xeb, 0xeb, 0xeb),
			RGB(0x28, 0x28, 0x28), RGB(0xd0, 0x4a, 0x00),
			RGB(0x87, 0x87, 0x87), RGB(0x47, 0x47, 0x47),
			RGB(0x00, 0x54, 0xcf), RGB(0x7f, 0x51, 0xd6),
			RGB(0xcd, 0xcd, 0xcd), RGB(0xff, 0xff, 0xff)
			}
		},
		{
		EMU_COLOR_THEME_WINDOWS,
		L"Windows Classic",
		VC_WHITE,
		VC_BLACK,
			{
			RGB(  0,   0,	0), RGB(  0,   0, 128),
			RGB(  0, 128,   0), RGB(  0, 128, 128),
			RGB(128,   0,   0), RGB(128,   0, 128),
			RGB(128, 128,  32), RGB(192, 192, 192),
			RGB(128, 128, 128), RGB(  0,   0, 255),
			RGB(  0, 255,   0), RGB(  0, 255, 255),
			RGB(255,   0,   0), RGB(255,   0, 255),
			RGB(255, 255,   0), RGB(255, 255, 255)
			}
		}
	};

static const STTERMCOLORTHEME *termFindColorTheme(int nTheme)
	{
	int i;

	for (i = 0; i < DIM(astTermColorThemes); ++i)
		{
		if (astTermColorThemes[i].nTheme == nTheme)
			return &astTermColorThemes[i];
		}
	return &astTermColorThemes[0];
	}

static BOOL AllocTxtBuf(ECHAR ***fpalpstr, int const sRows, int const sCols);
static BOOL AllocAttrBuf(PSTATTR **fpapst, const int sRows, const int sCols);
static void FreeTxtBuf(ECHAR ***fpalpstr, const int sRows);
static void FreeAttrBuf(PSTATTR **fpapst, const int sRows);
static BOOL termAllocBkBuf(const HHTERM hhTerm);
static void termFreeBkBuf(const HHTERM hhTerm);
static void GetDefaultHostWideFont(const HHTERM hhTerm);
static int APIENTRY EnumFontCallback(LPLOGFONT lplf, LPTEXTMETRIC lptm,
							DWORD dwType, LPVOID lpData);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	RegisterTerminalClass
 *
 * DESCRIPTION:
 *	Registers the terminal class.  Called in InitApplication()
 *
 * ARGUMENTS:
 *	hInstance	- Instance handle of app.
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL RegisterTerminalClass(const HINSTANCE hInstance)
	{
	WNDCLASS  wc;

	if (GetClassInfo(hInstance, TERM_CLASS, &wc) == FALSE)
		{
		wc.style		 = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc	 = (WNDPROC)TermProc;
		wc.cbClsExtra	 = 0;
		wc.cbWndExtra	 = sizeof(LONG_PTR);
		wc.hInstance	 = hInstance;
		wc.hIcon		 = 0;
		wc.hCursor		 = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = 0;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = TERM_CLASS;

		if (RegisterClass(&wc) == FALSE)
			{
			assert(FALSE);
			return FALSE;
			}
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateTerminalWindow
 *
 * DESCRIPTION:
 *	Creates a terminal window.
 *
 * ARGUMENTS:
 *	hwndSession - session window handle.
 *
 * RETURNS:
 *	hwnd or zero.
 *
 */
HWND CreateTerminalWindow(const HWND hwndSession)
	{
	HWND hwnd;

	hwnd = CreateWindowEx(
	  WS_EX_CLIENTEDGE,
	  TERM_CLASS,
	  "",
	  WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS | WS_VISIBLE,
	  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	  hwndSession,
	  (HMENU)IDC_TERMINAL_WIN,
	  glblQueryDllHinst(),
	  0
      );

	if (hwnd == 0)
		{
		assert(FALSE);
		return 0;
		}

	return hwnd;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateTerminalHdl
 *
 * DESCRIPTION:
 *	Creates an internal terminal handle.
 *
 * ARGUMENTS:
 *	hSession	- session handle
 *	hwndTerm	- terminal window handle.
 *
 * RETURNS:
 *	HHTERM or zero on error.
 *
 */
HHTERM CreateTerminalHdl(const HWND hwndTerm)
	{
	HHTERM hhTerm;

	hhTerm = (HHTERM)malloc(sizeof(*hhTerm));

	if (hhTerm == 0)
		{
		assert(FALSE);
		return 0;
		}

	memset(hhTerm, 0, sizeof(*hhTerm));
	hhTerm->hwnd = hwndTerm;
	hhTerm->hwndSession = GetParent(hwndTerm);
	hhTerm->hSession = (HSESSION)GetWindowLongPtr(GetParent(hwndTerm), GWLP_USERDATA);

	/* --- ProcessMessage() aquires a session handle from here. --- */

	SetWindowLongPtr(hwndTerm, 0, (LONG_PTR)hhTerm->hSession);

	hhTerm->iRows = 24; 	// standard, loading an emulator could change it.
	hhTerm->iCols = 80; 	// standard, loading an emulator could change it.

	hhTerm->iColorTheme = EMU_COLOR_THEME_SOLARIZED_LIGHT;
	hhTerm->pacrEmuColors = termFindColorTheme(hhTerm->iColorTheme)->acrColors;
	hhTerm->xBezel = BEZEL_SIZE;
	hhTerm->xIndent = 3;
	hhTerm->fCursorTracking = TRUE;
	hhTerm->fCursorsLinked = TRUE;
	hhTerm->fBlink = TRUE;
	hhTerm->uBlinkRate = GetCaretBlinkTime();
	hhTerm->iCurType = EMU_CURSOR_LINE;
	hhTerm->hUpdate = updateCreate(hhTerm->hSession);

	memset(hhTerm->underscores, '_', MAX_EMUCOLS);
	hhTerm->underscores[MAX_EMUCOLS-1] = L'\0';

	if (hhTerm->hUpdate == 0)
		return 0;

	// Allocate space for terminal text and attributes.

	if (AllocTxtBuf(&hhTerm->fplpstrTxt, MAX_EMUROWS, MAX_EMUCOLS) == FALSE)
		return 0;

	if (AllocAttrBuf(&hhTerm->fppstAttr, MAX_EMUROWS, MAX_EMUCOLS) == FALSE)
		return 0;

	if (termSysColorChng(hhTerm) == FALSE)
		return 0;

	if (termSetFont(hhTerm, 0) == FALSE)
		return 0;

	if (termAllocBkBuf(hhTerm) == FALSE)
		return 0;

	return hhTerm;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DestroyTerminalHdl
 *
 * DESCRIPTION:
 *	Gracefully cleans up the terminal handle.
 *
 * ARGUMENTS:
 *	hhTerm	- internal terminal handle.
 *
 * RETURNS:
 *	void
 *
 */
void DestroyTerminalHdl(const HHTERM hhTerm)
	{
	if (hhTerm == 0)
		return;

	if (hhTerm->hUpdate)
		updateDestroy(hhTerm->hUpdate);

	//TODO: destroy terminal window.

	FreeTxtBuf(&hhTerm->fplpstrTxt, MAX_EMUROWS);
	FreeAttrBuf(&hhTerm->fppstAttr, MAX_EMUROWS);

	/* --- Delete fonts --- */

	if (hhTerm->hFont)
		DeleteObject(hhTerm->hFont);

	//if (hhTerm->hUFont)
	//	  DeleteObject(hhTerm->hUFont);

	if (hhTerm->hDblHiFont)
		DeleteObject(hhTerm->hDblHiFont);

	//if (hhTerm->hDblHiUFont)
	//	  DeleteObject(hhTerm->hDblHiUFont);

	if (hhTerm->hDblWiFont)
		DeleteObject(hhTerm->hDblWiFont);

	//if (hhTerm->hDblWiUFont)
	//	  DeleteObject(hhTerm->hDblWiUFont);

	if (hhTerm->hDblHiWiFont)
		DeleteObject(hhTerm->hDblHiWiFont);

	//if (hhTerm->hDblHiWiUFont)
	//	  DeleteObject(hhTerm->hDblHiWiUFont);

	/* --- Delete alternate symbol fonts --- */

	if (hhTerm->hSymFont)
		DeleteObject(hhTerm->hSymFont);

	//if (hhTerm->hSymUFont)
	//	  DeleteObject(hhTerm->hSymUFont);

	if (hhTerm->hSymDblHiFont)
		DeleteObject(hhTerm->hSymDblHiFont);

	//if (hhTerm->hSymDblHiUFont)
	//	  DeleteObject(hhTerm->hSymDblHiUFont);

	if (hhTerm->hSymDblWiFont)
		DeleteObject(hhTerm->hSymDblWiFont);

	//if (hhTerm->hSymDblWiUFont)
	//	  DeleteObject(hhTerm->hSymDblWiUFont);

	if (hhTerm->hSymDblHiWiFont)
		DeleteObject(hhTerm->hSymDblHiWiFont);

	//if (hhTerm->hSymDblHiWiUFont)
	//	  DeleteObject(hhTerm->hSymDblHiWiUFont);

	/* --- Other stuff --- */

	if (hhTerm->hbrushTerminal)
		DeleteObject(hhTerm->hbrushTerminal);

	if (hhTerm->hDkGrayPen)
		DeleteObject(hhTerm->hDkGrayPen);

	if (hhTerm->hLtGrayPen)
		DeleteObject(hhTerm->hLtGrayPen);

	if (hhTerm->hbrushTermHatch)
		DeleteObject(hhTerm->hbrushTermHatch);

	if (hhTerm->hbrushBackHatch)
		DeleteObject(hhTerm->hbrushBackHatch);

	if (hhTerm->hbrushDivider)
		DeleteObject(hhTerm->hbrushDivider);

	if (hhTerm->hbrushHighlight)
		DeleteObject(hhTerm->hbrushHighlight);

	if (hhTerm->hCursorTimer)
		TimerDestroy(&hhTerm->hCursorTimer);

	if (hhTerm->fplpstrBkTxt)
		termFreeBkBuf(hhTerm);

	free(hhTerm);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	AllocTxtBuf
 *
 * DESCRIPTION:
 *	Allocates text buffer for terminal image.
 *
 * ARGUMENTS:
 *	fpalpstr	- pointer to pointer to buffer array.
 *	sRows		- number of rows
 *	sCols		- number of cols in each row.
 *
 * RETURNS:
 *	BOOL
 *
 */
static BOOL AllocTxtBuf(ECHAR ***fpalpstr, int const sRows, int const sCols)
	{
	register int i;

	FreeTxtBuf(fpalpstr, sRows); // free any old stuff

	if ((*fpalpstr = (ECHAR **)malloc((unsigned int)sRows * sizeof(ECHAR *))) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	memset(*fpalpstr, 0, (unsigned int)sRows * sizeof(ECHAR *));

	for (i = 0 ; i < sRows ; ++i)
		{
		if (((*fpalpstr)[i] = (ECHAR *)malloc(sizeof(ECHAR) * (unsigned int)sCols)) == 0)
			{
			FreeTxtBuf(fpalpstr, sRows);
			assert(FALSE);
			return FALSE;
			}

		ECHAR_Fill((*fpalpstr)[i], EMU_BLANK_CHAR, (unsigned int)sCols);
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	AllocAttrBuf
 *
 * DESCRIPTION:
 *	Allocates attribute buffer for terminal image.
 *
 * ARGUMENTS:
 *	fpapst		- pointer to pointer to buffer array.
 *	sRows		- number of rows
 *	sCols		- number of cols in each row.
 *
 * RETURNS:
 *	BOOL
 *
 */
static BOOL AllocAttrBuf(PSTATTR **fpapst, const int sRows, const int sCols)
	{
	register int i, j;
	STATTR stAttr;

	memset(&stAttr, 0, sizeof(STATTR));
	stAttr.txtclr = (unsigned int)GetNearestColorIndex(GetSysColor(COLOR_WINDOWTEXT));
	stAttr.bkclr =	(unsigned int)GetNearestColorIndex(GetSysColor(COLOR_WINDOW));

	FreeAttrBuf(fpapst, sRows);  // free any old stuff

	if ((*fpapst = (PSTATTR *)malloc((unsigned int)sRows * sizeof(PSTATTR))) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	for (i = 0 ; i < sRows ; ++i)
		{
		if (((*fpapst)[i] = (PSTATTR)malloc(sizeof(STATTR) * (unsigned int)sCols)) == 0)
			{
			FreeAttrBuf(fpapst, sRows);  // free any old stuff
			assert(FALSE);
			return FALSE;
			}

		for (j = 0 ; j < sCols ; ++j)
			MemCopy((*fpapst)[i]+j, &stAttr, sizeof(STATTR));
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	FreeTxtBuf
 *
 * DESCRIPTION:
 *	Free-up any allocated buffers for terminal text image
 *
 * ARGUMENTS:
 *	fpalpstr	- pointer to pointer to buffer array.
 *	sRows		- number of rows
 *	sCols		- number of cols in each row.
 *
 * RETURNS:
 *	void
 *
 */
static void FreeTxtBuf(ECHAR ***fpalpstr, const int sRows)
	{
	register int i;
	ECHAR **alpstr = *fpalpstr;

	if (alpstr)
		{
		for (i = 0 ; *alpstr && i < sRows ; ++i)
			{
			free(*alpstr);
			*alpstr = NULL;
			alpstr += 1;
			}

		free(*fpalpstr);
		*fpalpstr = 0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	FreeAttrBuf
 *
 * DESCRIPTION:
 *	Free-up any allocated buffers for terminal attribute image
 *
 * ARGUMENTS:
 *	fpapst		- pointer to pointer to buffer array.
 *	sRows		- number of rows
 *	sCols		- number of cols in each row.
 *
 * RETURNS:
 *	void
 *
 */
static void FreeAttrBuf(PSTATTR **fpapst, const int sRows)
	{
	register int i;
	PSTATTR *apst = *fpapst;

	if (apst)
		{
		for (i = 0 ; *apst && i < sRows ; ++i)
			{
			free(*apst);
			*apst = NULL;
			apst += 1;
			}

		free(*fpapst);
		*fpapst = 0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termSysColorChng
 *
 * DESCRIPTION:
 *	Creates new brushes because system colors have changed.  Also used
 *	during initialization of terminal window.
 *
 * ARGUMENTS:
 *	hhTerm	- internal terminal handle.
 *
 * RETURNS:
 *	void
 *
 */
BOOL termSysColorChng(const HHTERM hhTerm)
	{
	#define HATCH_PATTERN HS_BDIAGONAL

	HBRUSH hBrush;
	HPEN hPen;
	COLORREF cr;

	hhTerm->crBackScrl = GetSysColor(COLOR_BTNFACE);
	hhTerm->crBackScrlTxt = GetSysColor(COLOR_BTNTEXT);

	hhTerm->hBlackPen = GetStockObject(BLACK_PEN);
	hhTerm->hWhitePen = GetStockObject(WHITE_PEN);

	/* ----------------------- */

	cr = GetSysColor(COLOR_WINDOW);

	if ((hBrush = CreateSolidBrush(cr)) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	hhTerm->crTerm = cr;

	if (hhTerm->hbrushTerminal)
		DeleteObject(hhTerm->hbrushTerminal);

	hhTerm->hbrushTerminal = hBrush;

	/* ----------------------- */

	cr = GetSysColor(COLOR_BTNFACE);

	if ((hBrush = CreateSolidBrush(cr)) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	hhTerm->crBackScrl = cr;

	if (hhTerm->hbrushBackScrl)
		DeleteObject(hhTerm->hbrushBackScrl);

	hhTerm->hbrushBackScrl = hBrush;

	/* ----------------------- */

	cr = GetSysColor(COLOR_BTNSHADOW);

	if ((hBrush = CreateSolidBrush(cr)) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhTerm->hbrushDivider)
		DeleteObject(hhTerm->hbrushDivider);

	hhTerm->hbrushDivider = hBrush;

	/* ----------------------- */

	cr = GetSysColor(COLOR_HIGHLIGHT);

	if ((hBrush = CreateSolidBrush(cr)) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhTerm->hbrushHighlight)
		DeleteObject(hhTerm->hbrushHighlight);

	hhTerm->hbrushHighlight = hBrush;

	/* ----------------------- */

	cr = GetSysColor(COLOR_BTNFACE);

	if ((hPen = CreatePen(PS_SOLID, 0, cr)) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhTerm->hLtGrayPen)
		DeleteObject(hhTerm->hLtGrayPen);

	hhTerm->hLtGrayPen = hPen;

	/* ----------------------- */

	hBrush = CreateHatchBrush(HATCH_PATTERN, GetSysColor(COLOR_BTNFACE));

	if (hBrush == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhTerm->hbrushTermHatch)
		DeleteObject(hhTerm->hbrushTermHatch);

	hhTerm->hbrushTermHatch = hBrush;

	/* ----------------------- */

	hBrush = CreateHatchBrush(HATCH_PATTERN, GetSysColor(COLOR_BTNSHADOW));

	if (hBrush == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhTerm->hbrushBackHatch)
		DeleteObject(hhTerm->hbrushBackHatch);

	hhTerm->hbrushBackHatch = hBrush;

	/* ----------------------- */

	cr = GetSysColor(COLOR_BTNSHADOW);

	if ((hPen = CreatePen(PS_SOLID, 0, cr)) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	if (hhTerm->hDkGrayPen)
		DeleteObject(hhTerm->hDkGrayPen);

	hhTerm->hDkGrayPen = hPen;

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termSetFont
 *
 * DESCRIPTION:
 *	Sets the terminal font to the given font.  If hFont is zero,
 *	termSetFont() trys to create a default font.
 *
 * ARGUMENTS:
 *	hhTerm	- internal term handle.
 *	plf 	- pointer to logfont
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL termSetFont(const HHTERM hhTerm, const PLOGFONT plf)
	{
	HDC 	hdc;
	LOGFONT lf;
	HFONT	hFont;
	char	ach[256];
	TEXTMETRIC tm;
	int		nSize1, nSize2;
	int		nCharSet;

	if (plf == 0)
		{
		// No font, use the default font for UltraTerminal
        // (usually New Courier)

		memset(&lf, 0, sizeof(LOGFONT));

        // Someone added these different font sizes using the resource file
        // without consulting me.  Seems pretty stupid to do it this way.
        // - mrw:3/5/96

		// For VGA screens, we need a smaller font.
		//
        if (LoadString(glblQueryDllHinst(), IDS_TERM_DEF_VGA_SIZE,
                ach, sizeof(ach) / sizeof(WCHAR)))
            {
            nSize1 = (int)wcstol(ach, NULL, 10);
            }
		else
            {
			nSize1 = -11;
            }

        if (LoadString(glblQueryDllHinst(), IDS_TERM_DEF_NONVGA_SIZE,
                ach, sizeof(ach) / sizeof(WCHAR)))
            {
            nSize2 = (int)wcstol(ach, NULL, 10);
            }

		else
            {
			nSize2 = -15;
            }

		lf.lfHeight = (GetSystemMetrics(SM_CXSCREEN) < 810) ? nSize1 : nSize2;

        // mrw:3/5/96 - Font comes out really dinky on high res screens.
        //
        #if defined(INCL_USE_TERMINAL_FONT)
        if (GetSystemMetrics(SM_CXSCREEN) >= 1024)
            lf.lfHeight = -19;
        #endif

		lf.lfPitchAndFamily = FIXED_PITCH | MONO_FONT;

        if (LoadString(glblQueryDllHinst(), IDS_TERM_DEF_FONT,
                ach, sizeof(ach) / sizeof(WCHAR)))
            {
            lstrcpynW(lf.lfFaceName, ach,
                    sizeof(lf.lfFaceName) / sizeof(WCHAR));
            lf.lfFaceName[sizeof(lf.lfFaceName)/sizeof(WCHAR)-1] = L'\0';
            }

        if (LoadString(glblQueryDllHinst(), IDS_TERM_DEF_CHARSET,
                ach, sizeof(ach) / sizeof(WCHAR)))
            {
            nCharSet = (int)wcstol(ach, NULL, 10);
			lf.lfCharSet = (BYTE)nCharSet;
            }

		hFont = CreateFontIndirect(&lf);

		if (hFont == 0)
			{
			//*lf.lfCharSet = ANSI_CHARSET;

			//*hFont = CreateFontIndirect(&lf);

			//*if (hFont == 0)
				{
				assert(FALSE);
				return FALSE;
				}
			}
		}

	else
		{
		hFont = CreateFontIndirect(plf);

		if (hFont == 0)
			{
			assert(FALSE);
			return FALSE;
			}
		}

	/* --- Ok, we have our base font, blast the previous fonts --- */

	if (hhTerm->hFont)
		{
		DeleteObject(hhTerm->hFont);
		hhTerm->hFont = 0;
		}


	if (hhTerm->hDblHiFont)
		{
		DeleteObject(hhTerm->hDblHiFont);
		hhTerm->hDblHiFont = 0;
		}

	if (hhTerm->hDblWiFont)
		{
		DeleteObject(hhTerm->hDblWiFont);
		hhTerm->hDblWiFont = 0;
		}

	if (hhTerm->hDblHiWiFont)
		{
		DeleteObject(hhTerm->hDblHiWiFont);
		hhTerm->hDblHiWiFont = 0;
		}

	/* --- And the symbol fonts --- */

	if (hhTerm->hSymFont)
		{
		DeleteObject(hhTerm->hSymFont);
		hhTerm->hSymFont = 0;
		}

	if (hhTerm->hSymDblHiFont)
		{
		DeleteObject(hhTerm->hSymDblHiFont);
		hhTerm->hSymDblHiFont = 0;
		}

	if (hhTerm->hSymDblWiFont)
		{
		DeleteObject(hhTerm->hSymDblWiFont);
		hhTerm->hSymDblWiFont = 0;
		}

	if (hhTerm->hSymDblHiWiFont)
		{
		DeleteObject(hhTerm->hSymDblHiWiFont);
		hhTerm->hSymDblHiWiFont = 0;
		}

	/* --- Commit to the new font --- */

	hhTerm->hFont = hFont;

	if (GetObject(hFont, sizeof(LOGFONT), &lf) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- save what we really got --- */

	hhTerm->lf = lf;

	/* --- Get size of selected font. --- */

	hdc = GetDC(hhTerm->hwnd);
	hFont = (HFONT)SelectObject(hdc, hFont);

	GetTextMetrics(hdc, &tm);
	SelectObject(hdc, hFont);

	ReleaseDC(hhTerm->hwnd, hdc);

	hhTerm->xChar = tm.tmAveCharWidth;
	hhTerm->yChar = tm.tmHeight;

    #if 0
	if ((tm.tmMaxCharWidth % 2) == 0)
		{
		hhTerm->iEvenFont = TRUE;
		}
	else
		{
		hhTerm->iEvenFont = FALSE;
		}

    #else
    hhTerm->iEvenFont = TRUE;   //mrw:10/10/95
    #endif

	// We need to know if the font is italic because it changes the
	// way we draw.  The italic fonts are regular fonts sheared.  The
	// shear causes the character to draw into the next text box.
	// This really messes us up.  To get around the problem, when
	// we're italic, we simply repaint the whole line when an text
	// comes in. - mrw,12/19/94
	//
	hhTerm->fItalic = tm.tmItalic;

	/* --- Set bezel size based on font --- */

	hhTerm->xBezel = BEZEL_SIZE;

	if (hhTerm->yChar < BEZEL_SIZE)
		{
		hhTerm->xBezel = max(5+OUTDENT, hhTerm->yChar);
		}

	switch (hhTerm->iCurType)
		{
	case EMU_CURSOR_LINE:
	default:
		hhTerm->iHstCurSiz = GetSystemMetrics(SM_CYBORDER) * 2;
		break;

	case EMU_CURSOR_BLOCK:
		hhTerm->iHstCurSiz = hhTerm->yChar;
		break;

	case EMU_CURSOR_NONE:
		hhTerm->iHstCurSiz = 0;
		break;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termMakeFont
 *
 * DESCRIPTION:
 *	When user selects a font, we only create the standard non-underlined
 *	font.  If the paint routines encounter an attribute that needs a
 *	a different font, then we create it on the spot.  This is more
 *	economic since often we won't need all 8 fonts for a given session.
 *
 * ARGUMENTS:
 *	hhTerm		- internal terminal handle
 *	fUnderline	- font is underlined
 *	fHigh		- font is double high
 *	fWide		- font is double wide
 *
 * RETURNS:
 *	0 on error, hFont on success.
 *
 */
HFONT termMakeFont(const HHTERM hhTerm, const BOOL fUnderline,
				   const BOOL fHigh, const BOOL fWide, const BOOL fSymbol)
	{
	LOGFONT lf;
	HFONT	hFont;

	lf = hhTerm->lf;
	lf.lfWidth = hhTerm->xChar;

	if (fSymbol)
		{
		//lf.lfCharSet = SYMBOL_CHARSET;
		StrCharCopy(lf.lfFaceName, "Arial Alternative Symbol");
		}

	if (fUnderline)
		lf.lfUnderline = 1;

	if (fHigh)
		lf.lfHeight *= 2;

	if (fWide)
		lf.lfWidth *= 2;

	if ((hFont = CreateFontIndirect(&lf)) == 0)
		assert(FALSE);

	return hFont;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fAllocBkBuf
 *
 * DESCRIPTION:
 *	Allocates buffer space for backscroll text
 *
 * ARGUMENTS:
 *	hhTerm	- internal terminal handle
 *
 * RETURNS:
 *	BOOL
 *
 */
static BOOL termAllocBkBuf(const HHTERM hhTerm)
	{
	register int i;

	// This number should be big enough so that a maximized window of
	// backscroll text can be displayed.

	hhTerm->iMaxPhysicalBkRows = hhTerm->iPhysicalBkRows = min(5000,
		(GetSystemMetrics(SM_CYFULLSCREEN) / hhTerm->yChar) + 1);

	if ((hhTerm->fplpstrBkTxt = malloc((unsigned int)hhTerm->iMaxPhysicalBkRows *
			sizeof(ECHAR *))) == 0)
		{
		return FALSE;
		}

	for (i = 0 ; i < hhTerm->iMaxPhysicalBkRows ; ++i)
		{
		if ((hhTerm->fplpstrBkTxt[i] =
				malloc(MAX_EMUCOLS * sizeof(ECHAR))) == 0)
			{
			termFreeBkBuf(hhTerm);
			return FALSE;
			}

		ECHAR_Fill(hhTerm->fplpstrBkTxt[i], EMU_BLANK_CHAR, MAX_EMUCOLS);
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	FreeBkBuf
 *
 * DESCRIPTION:
 *	Frees space allocated to backscroll buffer.
 *
 * ARGUMENTS:
 *	hhTerm	- internal terminal handle
 *
 * RETURNS:
 *	void
 *
 */
static void termFreeBkBuf(const HHTERM hhTerm)
	{
	register int i;

	for (i = 0 ; i < hhTerm->iMaxPhysicalBkRows ; ++i)
		{
		if (hhTerm->fplpstrBkTxt[i])
			{
			free(hhTerm->fplpstrBkTxt[i]);
			hhTerm->fplpstrBkTxt[i] = NULL;
			}
		}

	free(hhTerm->fplpstrBkTxt);
	hhTerm->fplpstrBkTxt = NULL;
	return;
	}

const WCHAR *termColorThemeName(int nTheme)
	{
	return termFindColorTheme(nTheme)->pszName;
	}

int termColorThemeDefaultText(int nTheme)
	{
	return termFindColorTheme(nTheme)->nDefaultText;
	}

int termColorThemeDefaultBackground(int nTheme)
	{
	return termFindColorTheme(nTheme)->nDefaultBackground;
	}

COLORREF termColorThemeColor(int nTheme, int nColor)
	{
	if (nColor < 0 || nColor >= MAX_EMUCOLORS)
		nColor = 0;
	return termFindColorTheme(nTheme)->acrColors[nColor];
	}

void termApplyColorTheme(const HHTERM hhTerm, int nTheme)
	{
	if (hhTerm == 0)
		return;

	hhTerm->iColorTheme = termFindColorTheme(nTheme)->nTheme;
	hhTerm->pacrEmuColors = termFindColorTheme(hhTerm->iColorTheme)->acrColors;
	termSetClrAttr(hhTerm);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	GetNearestColorIndex
 *
 * DESCRIPTION:
 *	Duplicates a palette function that I couldn't get to work.  Basicly,
 *	the emulator has a table of colors it is allowed to use.  When the
 *	user picks a color, this function returns the index of the color that
 *	most closely matches.  How does it do this you say?  Well, imagine
 *	the problem as 3D space.  The pallete colors all map into this space.
 *	The goal is to find the pallete color closest to the given colorref
 *	value.	Borrowing from the 10th grade algrebra we know that:
 *
 *		X^2 + Y^2 + Z^2 = C^2
 *
 *	The distance between two points is then:
 *
 *		(X - X')^2 + (Y - Y')^2 + (Z - Z') = C'^2
 *
 *	The point with the smallest C'^2 value wins!
 *
 * ARGUMENTS:
 *	COLORREF *acr	- color table to use for matching
 *	COLORREF cr 	- color to match.
 *
 * RETURNS:
 *	An index of the closest matching color.
 *
 */
int GetNearestColorIndex(COLORREF cr)
	{
	int   i, idx = 0;
	unsigned int  R, G, B;
	unsigned long C, CMin = (unsigned long)-1;

	for (i = 0 ; i < MAX_EMUCOLORS ; ++i)
		{
		const COLORREF crTheme =
				termFindColorTheme(EMU_COLOR_THEME_SOLARIZED_LIGHT)->acrColors[i];
		R = GetRValue(crTheme) - GetRValue(cr);	R *= R;
		G = GetGValue(crTheme) - GetGValue(cr);	G *= G;
		B = GetBValue(crTheme) - GetBValue(cr);	B *= B;

		C = (ULONG)(R + G + B);

		if (C < CMin)
			{
			CMin = C;
			idx = i;

			if (C == 0) 	// we matched!
				break;
			}
		}

	return idx;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	UnregisterTerminalClass
 *
 * DESCRIPTION:
 *	unregisters the terminal class.  Called in InitApplication()
 *
 * ARGUMENTS:
 *	hInstance	- Instance handle of app.
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL UnregisterTerminalClass(const HINSTANCE hInstance)
	{
	return UnregisterClass(TERM_CLASS, hInstance);
	}
