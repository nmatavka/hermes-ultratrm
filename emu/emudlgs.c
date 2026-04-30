/*	File: D:\WACKER\emu\emudlg.c (Created: 14-Feb-1994)
 *
 *	Copyright 1991, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 10 $
 *	$Date: 3/16/01 4:28p $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <tdll/stdtyp.h>
#include <tdll/tdll.h>
#include <tdll/assert.h>
#include <tdll/session.h>
#include <tdll/statusbr.h>
#include <tdll/misc.h>
#include <term/res.h>
#include <tdll/globals.h>
#include <tdll/load_res.h>
#include <tdll/ut_text.h>
#include <tdll/hlptable.h>
#include <tdll/term.h>

#include "emu.h"
#include "emuid.h"
#include "emudlgs.h"

// Static function prototypes...
//
STATIC_FUNC void emudlgInitCursorSettings  (HWND hDlg,
									  		PSTEMUSET	pstEmuSettings,
									  		INT  ID_UNDERLINE,
									  		INT  ID_BLOCK,
									  		INT  ID_BLINK);
STATIC_FUNC void emudlgInitCharSetSetting(HWND  hDlg,
											PSTEMUSET pstEmuSettings,
											int nCharSetTableID,
											int nDefaultCharSetID);
STATIC_FUNC BOOL emudlgFindCharSetName(HWND  hDlg,
										BYTE *pbCharSetTable,
										int nCharSetID,
										LPWSTR *ppszCharSetName,
										BOOL fTellDlg);
#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
STATIC_FUNC void emudlgInitRowsCols(HWND hDlg, PSTEMUSET pstEmuSettings);
STATIC_FUNC void emudlgGetRowColSettings(HWND hDlg, PSTEMUSET pstEmuSettings);
STATIC_FUNC void emudlgCreateUpDownControl(HWND hDlg,
										PSTEMUSET pstEmuSettings);
STATIC_FUNC int emudlgValidateEntryFieldSetting(HWND hDlg,
										int nIDC,
										int nMinVal,
										int nMaxVal);
STATIC_FUNC void emudlgInitColorThemeCombo(HWND hDlg, PSTEMUSET pstEmuSettings);
STATIC_FUNC int emudlgColorFromControlId(int nId, BOOL *pfText);
STATIC_FUNC void emudlgUpdateColorPreview(HWND hDlg);
STATIC_FUNC BOOL emudlgDrawColorButton(HWND hDlg, const DRAWITEMSTRUCT *pdis);
#endif

// Defines...
//
#define IDC_KEYPAD_MODE					104
#define IDC_CURSOR_MODE					106
#define IDC_132_COLUMN					107
#define IDC_TF_CHARACTER_SET			109
#define IDC_CHARACTER_SET				110
#define IDC_BLOCK_CURSOR				112
#define IDC_UNDERLINE_CURSOR			113
#define IDC_BLINK_CURSOR				114
#define IDC_DESTRUCTIVE_BKSP			116
#define IDC_ALT_MODE					117
#define IDC_SEND_POUND_SYMBOL			122
#define IDC_HIDE_CURSOR					119
#define IDC_GR_CURSOR					111
#define IDC_GR_TERMINAL_MODES			118
#define IDC_USE_8_BIT_CODES				120
#define	IDC_ALLOW_USERDEFINED_KEYS		121
#define IDC_GR_SCREEN					130
#define IDC_TF_ROWS						131
#define	IDC_NUMBER_OF_ROWS				132
#define IDC_TF_COLUMNS					133
#define	IDC_NUMBER_OF_COLS				134
#define	IDC_PRINT_RAW					135

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
typedef struct stColorDlg
	{
	PSTEMUSET pstEmuSettings;
	HBRUSH hPreviewBrush;
	} STCOLORDLG;
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuSettingsDlg
 *
 * DESCRIPTION:
 *	Decide which emulator settings dialog to call.
 *
 * ARGUMENTS:
 *  hSession 	   - the session handle.
 *  nEmuId		   - emulator id.
 *  pstEmuSettings - settings structure to fill in.  It should be initialized
 *					 up above.
 *
 * RETURNS:
 *	fResult - return value from the DoDialog().
 *
 */
BOOL emuSettingsDlg(const HSESSION hSession, const HWND hwndParent,
					const int nEmuId, PSTEMUSET pstEmuSettings)
	{
	BOOL		fResult = FALSE;

	assert(hSession && hwndParent);

	switch (nEmuId)
		{
	case EMU_ANSIW:
	case EMU_ANSI:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_ANSI_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuANSI_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

	case EMU_TTY:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_TTY_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuTTY_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

	case EMU_VT52:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT52_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT52_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

	case EMU_VT100:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT100_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT100_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

	case EMU_VT100J:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT100J_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT100J_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

#if defined(INCL_VT220)
	case EMU_VT220:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT220_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT220_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;
#endif

#if defined(INCL_VT320)
	case EMU_VT320:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT220_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT220_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;
#endif

#if defined(INCL_VT100PLUS)
	case EMU_VT100PLUS:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT100J_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT100_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;
#endif

#if defined(INCL_VTUTF8)
	case EMU_VTUTF8:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VT100J_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuVT100_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;
#endif

	case EMU_MINI:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_MINITEL_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuMinitel_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

	case EMU_VIEW:
		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
								 MAKEINTRESOURCE(IDD_VIEWDATA_SETTINGS),
								 hwndParent,
								 (DLGPROC)emuViewdata_SettingsDlgProc,
								 (LPARAM)pstEmuSettings);
		break;

	default:
		break;
		}

	return fResult;
	}

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
int emuColorSettingsDlg(const HSESSION hSession,
					const HWND hwndParent, PSTEMUSET pstEmuSettings)
	{
	STEMUSET stWorkingSettings;
	INT_PTR nResult;

	(void)hSession;
	if (pstEmuSettings == NULL)
		return IDCANCEL;

	memcpy(&stWorkingSettings, pstEmuSettings, sizeof(stWorkingSettings));
	nResult = DoDialog(glblQueryDllHinst(),
			MAKEINTRESOURCE(IDD_SCRN_COLOR_SETTINGS),
			hwndParent,
			(DLGPROC)emuColorSettingsDlgProc,
			(LPARAM)&stWorkingSettings);

	if (nResult == IDOK)
		{
		memcpy(pstEmuSettings, &stWorkingSettings, sizeof(*pstEmuSettings));
		return IDOK;
		}

	return IDCANCEL;
	}

INT_PTR CALLBACK emuColorSettingsDlgProc(HWND hDlg, UINT wMsg,
		WPARAM wPar, LPARAM lPar)
	{
	STCOLORDLG *pstDlg;
	PSTEMUSET pstEmuSettings;
	BOOL fText;
	int nColor;
	int nSel;
	HBRUSH hBrush;
	HWND hwndPreview;

	pstDlg = (STCOLORDLG *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstDlg = (STCOLORDLG *)calloc(1, sizeof(*pstDlg));
		if (pstDlg == NULL)
			{
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
			}

		pstDlg->pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstDlg);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));
		emudlgInitColorThemeCombo(hDlg, pstDlg->pstEmuSettings);
		emudlgUpdateColorPreview(hDlg);
		return TRUE;

	case WM_DESTROY:
		if (pstDlg)
			{
			if (pstDlg->hPreviewBrush)
				DeleteObject(pstDlg->hPreviewBrush);
			free(pstDlg);
			SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
			}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (pstDlg && pstDlg->pstEmuSettings &&
				(HWND)lPar == GetDlgItem(hDlg, IDC_TEXT_PREVIEW))
			{
			pstEmuSettings = pstDlg->pstEmuSettings;
			SetTextColor((HDC)wPar, termColorThemeColor(
					pstEmuSettings->nColorTheme,
					pstEmuSettings->nTextColor));
			SetBkColor((HDC)wPar, termColorThemeColor(
					pstEmuSettings->nColorTheme,
					pstEmuSettings->nBackgroundColor));
			if (pstDlg->hPreviewBrush == NULL)
				pstDlg->hPreviewBrush = CreateSolidBrush(
						termColorThemeColor(pstEmuSettings->nColorTheme,
								pstEmuSettings->nBackgroundColor));
			return (INT_PTR)pstDlg->hPreviewBrush;
			}
		break;

	case WM_DRAWITEM:
		if (emudlgDrawColorButton(hDlg, (const DRAWITEMSTRUCT *)lPar))
			return TRUE;
		break;

	case WM_COMMAND:
		if (pstDlg == NULL || pstDlg->pstEmuSettings == NULL)
			break;
		pstEmuSettings = pstDlg->pstEmuSettings;

		switch (LOWORD(wPar))
			{
		case IDC_COLOR_THEME_COMBO:
			if (HIWORD(wPar) == CBN_SELCHANGE)
				{
				nSel = (int)SendDlgItemMessage(hDlg,
						IDC_COLOR_THEME_COMBO, CB_GETCURSEL, 0, 0);
				if (nSel != CB_ERR)
					{
					pstEmuSettings->nColorTheme =
							(int)SendDlgItemMessage(hDlg,
									IDC_COLOR_THEME_COMBO,
									CB_GETITEMDATA, (WPARAM)nSel, 0);
					pstEmuSettings->nTextColor =
							termColorThemeDefaultText(
									pstEmuSettings->nColorTheme);
					pstEmuSettings->nBackgroundColor =
							termColorThemeDefaultBackground(
									pstEmuSettings->nColorTheme);
					emudlgUpdateColorPreview(hDlg);
					}
				}
			return TRUE;

		case IDOK:
			EndDialog(hDlg, IDOK);
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;

		default:
			nColor = emudlgColorFromControlId(LOWORD(wPar), &fText);
			if (nColor >= 0 && HIWORD(wPar) == BN_CLICKED)
				{
				if (fText)
					pstEmuSettings->nTextColor = nColor;
				else
					pstEmuSettings->nBackgroundColor = nColor;
				emudlgUpdateColorPreview(hDlg);
				return TRUE;
				}
			break;
			}
		break;

	default:
		break;
		}

	hwndPreview = GetDlgItem(hDlg, IDC_TEXT_PREVIEW);
	if (wMsg == WM_CTLCOLORSTATIC && hwndPreview == (HWND)lPar)
		{
		hBrush = GetSysColorBrush(COLOR_WINDOW);
		return (INT_PTR)hBrush;
		}

	return FALSE;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuANSI_SettingsDlgProc
 *
 * DESCRIPTION:
 *	ANSI Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuANSI_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	static		DWORD aHlpTable[] =
		{
		IDC_BLOCK_CURSOR,		IDH_TERM_EMUSET_CURSOR,
		IDC_UNDERLINE_CURSOR,	IDH_TERM_EMUSET_CURSOR,
		IDC_BLINK_CURSOR,		IDH_TERM_EMUSET_CURSOR,
		IDC_GR_CURSOR,			IDH_TERM_EMUSET_CURSOR,
        IDCANCEL,                           IDH_CANCEL,
        IDOK,                               IDH_OK,
		0,						0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
		doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuTTY_SettingsDlgProc
 *
 * DESCRIPTION:
 *	TTY Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuTTY_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	static		DWORD aHlpTable[] = {IDC_DESTRUCTIVE_BKSP,	IDH_TERM_EMUSET_DESTRUCTIVE,
									 IDC_BLOCK_CURSOR,		IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR,	IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,		IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 		IDH_TERM_EMUSET_CURSOR,
                                     IDCANCEL,                           IDH_CANCEL,
                                     IDOK,                               IDH_OK,
									 0, 					0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif

		/* -------------- Destructive Backspace ------------- */

		SendDlgItemMessage(hDlg, IDC_DESTRUCTIVE_BKSP, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fDestructiveBk, 0);

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Destructive Backspace ------------- */

			pstEmuSettings->fDestructiveBk =
				(int)IsDlgButtonChecked(hDlg, IDC_DESTRUCTIVE_BKSP);

			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuVT52_SettingsDlgProc
 *
 * DESCRIPTION:
 *	VT52 Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuVT52_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	static 		DWORD aHlpTable[] = {IDC_ALT_MODE,		   IDH_TERM_EMUSET_ALTMODE,
									 IDC_BLOCK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR, IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 	   IDH_TERM_EMUSET_CURSOR,
                                     IDCANCEL,                           IDH_CANCEL,
                                     IDOK,                               IDH_OK,
									 0, 					0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif

		/* -------------- Alternate keypad mode ------------- */

		SendDlgItemMessage(hDlg, IDC_ALT_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fAltKeypadMode, 0);

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Alternate keypad mode ------------- */

			pstEmuSettings->fAltKeypadMode =
				(int)IsDlgButtonChecked(hDlg, IDC_ALT_MODE);

			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuVT100_SettingsDlgProc
 *
 * DESCRIPTION:
 *	VT100 Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuVT100_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	int			nIndex;
	PSTEMUSET	pstEmuSettings;
	static 		DWORD aHlpTable[] = {IDC_KEYPAD_MODE,	   IDH_TERM_EMUSET_KEYPADMODE,
									 IDC_CURSOR_MODE,	   IDH_TERM_EMUSET_CURSORMODE,
									 IDC_132_COLUMN,	   IDH_TERM_EMUSET_132COLUMNS,
									 IDC_GR_TERMINAL_MODES,IDH_TERM_EMUSET_MODES,
									 IDC_CHARACTER_SET,	   IDH_TERM_EMUSET_CHARSETS,
									 IDC_TF_CHARACTER_SET, IDH_TERM_EMUSET_CHARSETS,
									 IDC_BLOCK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR, IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 	   IDH_TERM_EMUSET_CURSOR,
#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
									 IDC_GR_SCREEN,			IDH_TERM_EMUSET_ROWSANDCOLS,	
									 IDC_TF_ROWS,			IDH_TERM_EMUSET_ROWSANDCOLS,	
									 IDC_NUMBER_OF_ROWS,	IDH_TERM_EMUSET_ROWSANDCOLS,
									 IDC_TF_COLUMNS,		IDH_TERM_EMUSET_ROWSANDCOLS,
									 IDC_NUMBER_OF_COLS,	IDH_TERM_EMUSET_ROWSANDCOLS,
#endif
                                     IDCANCEL,                           IDH_CANCEL,
                                     IDOK,                               IDH_OK,
									 0, 					0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif

		/* -------------- Keypad application mode ------------- */

		SendDlgItemMessage(hDlg, IDC_KEYPAD_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fKeypadAppMode, 0);

		/* -------------- Cursor keypad mode ------------- */

		SendDlgItemMessage(hDlg, IDC_CURSOR_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fCursorKeypadMode, 0);

#if !defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- 132 Column Mode ------------- */

		SendDlgItemMessage(hDlg, IDC_132_COLUMN, BM_SETCHECK,
			(unsigned int)pstEmuSettings->f132Columns, 0);
#endif

		/* -------------- Host Print Mode ------------- */
		// mpt;04-22-00 added interface to control host-controlled printing
		SendDlgItemMessage(hDlg, IDC_PRINT_RAW, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fPrintRaw, 0);

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		/* -------------- VT100 Character Sets ------------- */

		emudlgInitCharSetSetting(hDlg, pstEmuSettings,
				IDT_EMU_VT100_CHAR_SETS, EMU_CHARSET_ASCII);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
		doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Keypad Application mode ------------- */

			pstEmuSettings->fKeypadAppMode =
				(int)IsDlgButtonChecked(hDlg, IDC_KEYPAD_MODE);

			/* -------------- Cursor Keypad Mode ------------- */

			pstEmuSettings->fCursorKeypadMode =
				(int)IsDlgButtonChecked(hDlg, IDC_CURSOR_MODE);

#if !defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- 132 Column Mode ------------- */

			pstEmuSettings->f132Columns =
				(int)IsDlgButtonChecked(hDlg, IDC_132_COLUMN);
#endif

			/* -------------- Host Print Mode ------------- */
			//mpt:04-22-00
			pstEmuSettings->fPrintRaw =
				(int)IsDlgButtonChecked(hDlg, IDC_PRINT_RAW);

			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			/* -------------- VT100 Character Set ------------- */

            nIndex = (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_GETCURSEL, 0, 0);
            assert(nIndex != CB_ERR);

            //JMH 01-09-97 Get the nCharacterSet value associated with this entry
            //
            pstEmuSettings->nCharacterSet =
                (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_GETITEMDATA,
                    (WPARAM) nIndex, (LPARAM) 0);
            assert(pstEmuSettings->nCharacterSet != CB_ERR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuVT100J_SettingsDlgProc
 *
 * DESCRIPTION:
 *	VT100 Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuVT100J_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	int			nIndex;
	static 		DWORD aHlpTable[] = {IDC_KEYPAD_MODE,	   IDH_TERM_EMUSET_KEYPADMODE,
									 IDC_CURSOR_MODE,	   IDH_TERM_EMUSET_CURSORMODE,
									 IDC_132_COLUMN,	   IDH_TERM_EMUSET_132COLUMNS,
									 IDC_GR_TERMINAL_MODES,IDH_TERM_EMUSET_MODES,
									 IDC_CHARACTER_SET,	   IDH_TERM_EMUSET_CHARSETS,
									 IDC_TF_CHARACTER_SET, IDH_TERM_EMUSET_CHARSETS,
									 IDC_BLOCK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR, IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 	   IDH_TERM_EMUSET_CURSOR,
                                     IDCANCEL,                           IDH_CANCEL,
                                     IDOK,                               IDH_OK,
									 0, 					0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif

		/* -------------- Keypad application mode ------------- */

		SendDlgItemMessage(hDlg, IDC_KEYPAD_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fKeypadAppMode, 0);

		/* -------------- Cursor keypad mode ------------- */

		SendDlgItemMessage(hDlg, IDC_CURSOR_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fCursorKeypadMode, 0);

#if !defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- 132 Column Mode ------------- */

		SendDlgItemMessage(hDlg, IDC_132_COLUMN, BM_SETCHECK,
			(unsigned int)pstEmuSettings->f132Columns, 0);
#endif

		/* -------------- Host Print Mode ------------- */
		//mpt:04-22-00
		SendDlgItemMessage(hDlg, IDC_PRINT_RAW, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fPrintRaw, 0);

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		/* -------------- VT100 Character Sets ------------- */

		emudlgInitCharSetSetting(hDlg, pstEmuSettings,
				IDT_EMU_VT100_CHAR_SETS, EMU_CHARSET_ASCII);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Keypad Application mode ------------- */

			pstEmuSettings->fKeypadAppMode =
				(int)IsDlgButtonChecked(hDlg, IDC_KEYPAD_MODE);

			/* -------------- Cursor Keypad Mode ------------- */

			pstEmuSettings->fCursorKeypadMode =
				(int)IsDlgButtonChecked(hDlg, IDC_CURSOR_MODE);

#if !defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- 132 Column Mode ------------- */

			pstEmuSettings->f132Columns =
				(int)IsDlgButtonChecked(hDlg, IDC_132_COLUMN);
#endif
			/* -------------- Host Print Mode ------------- */

			pstEmuSettings->fPrintRaw =
				(int)IsDlgButtonChecked(hDlg, IDC_PRINT_RAW);

			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			/* -------------- VT100 Character Set ------------- */

            nIndex = (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_GETCURSEL, 0, 0);
            assert(nIndex != CB_ERR);

            //JMH 01-09-97 Get the nCharacterSet value associated with this entry
            //
            pstEmuSettings->nCharacterSet =
                (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_GETITEMDATA,
                    (WPARAM) nIndex, (LPARAM) 0);
            assert(pstEmuSettings->nCharacterSet != CB_ERR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

#if defined(INCL_VT220)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuVT220_SettingsDlgProc
 *
 * DESCRIPTION:
 *	VT220 Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuVT220_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	int			nIndex;
	static 		DWORD aHlpTable[] = {IDC_KEYPAD_MODE,			IDH_TERM_EMUSET_KEYPADMODE,
									 IDC_CURSOR_MODE,			IDH_TERM_EMUSET_CURSORMODE,
									 IDC_132_COLUMN,			IDH_TERM_EMUSET_132COLUMNS,
									 IDC_GR_TERMINAL_MODES,		IDH_TERM_EMUSET_MODES,
									 IDC_CHARACTER_SET,			IDH_TERM_EMUSET_CHARSETS,
									 IDC_TF_CHARACTER_SET,		IDH_TERM_EMUSET_CHARSETS,
									 IDC_BLOCK_CURSOR,			IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR,		IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,			IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 			IDH_TERM_EMUSET_CURSOR,
									 IDC_USE_8_BIT_CODES,		IDH_TERM_EMUSET_8BITCODES,			
									 IDC_ALLOW_USERDEFINED_KEYS,IDH_TERM_EMUSET_USERDEFKEYS,
									 IDC_PRINT_RAW,				IDH_TERM_EMUSET_PRINTRAW,
                                     IDCANCEL,                           IDH_CANCEL,
                                     IDOK,                               IDH_OK,
									 0, 						0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif
		/* -------------- Keypad application mode ------------- */

		SendDlgItemMessage(hDlg, IDC_KEYPAD_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fKeypadAppMode, 0);

		/* -------------- Cursor keypad mode ------------- */

		SendDlgItemMessage(hDlg, IDC_CURSOR_MODE, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fCursorKeypadMode, 0);

#if !defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- 132 Column Mode ------------- */

		SendDlgItemMessage(hDlg, IDC_132_COLUMN, BM_SETCHECK,
			(unsigned int)pstEmuSettings->f132Columns, 0);
#endif
		/* -------------- Host Print Mode ------------- */

		SendDlgItemMessage(hDlg, IDC_PRINT_RAW, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fPrintRaw, 0);

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		/* -------------- 8 bit codes mode ------------- */

		SendDlgItemMessage(hDlg, IDC_USE_8_BIT_CODES, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fUse8BitCodes, 0);

		/* -------------- User defined keys ------------- */

		SendDlgItemMessage(hDlg, IDC_ALLOW_USERDEFINED_KEYS, BM_SETCHECK,
			(unsigned int)pstEmuSettings->fAllowUserKeys, 0);

		/* -------------- VT220 Character Sets ------------- */

		emudlgInitCharSetSetting(hDlg, pstEmuSettings,
				IDT_EMU_VT220_CHAR_SETS, EMU_CHARSET_MULTINATIONAL);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
		doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Keypad Application mode ------------- */

			pstEmuSettings->fKeypadAppMode =
				(int)IsDlgButtonChecked(hDlg, IDC_KEYPAD_MODE);

			/* -------------- Cursor Keypad Mode ------------- */

			pstEmuSettings->fCursorKeypadMode =
				(int)IsDlgButtonChecked(hDlg, IDC_CURSOR_MODE);

#if !defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- 132 Column Mode ------------- */

			pstEmuSettings->f132Columns =
				(int)IsDlgButtonChecked(hDlg, IDC_132_COLUMN);
#endif
			/* -------------- Host Print Mode ------------- */

			pstEmuSettings->fPrintRaw =
				(int)IsDlgButtonChecked(hDlg, IDC_PRINT_RAW);

			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			/* -------------- 8 bit codes mode ------------- */

			pstEmuSettings->fUse8BitCodes =
					(int)IsDlgButtonChecked(hDlg, IDC_USE_8_BIT_CODES);

			/* -------------- User defined keys ------------- */

			pstEmuSettings->fAllowUserKeys =
					(int)IsDlgButtonChecked(hDlg, IDC_ALLOW_USERDEFINED_KEYS);

			/* -------------- VT220 Character Set ------------- */

            nIndex = SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_GETCURSEL, 0, 0);
            assert(nIndex != CB_ERR);

            //JMH 01-09-97 Get the nCharacterSet value associated with this entry
            //
            pstEmuSettings->nCharacterSet =
                SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_GETITEMDATA,
                    (WPARAM) nIndex, (LPARAM) 0);
            assert(pstEmuSettings->nCharacterSet != CB_ERR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuMinitel_SettingsDlgProc
 *
 * DESCRIPTION:
 *	TTY Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuMinitel_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	static 		DWORD aHlpTable[] = {IDC_DESTRUCTIVE_BKSP, IDH_TERM_EMUSET_DESTRUCTIVE,
									 IDC_BLOCK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR, IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,	   IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 	   IDH_TERM_EMUSET_CURSOR,
                                     IDCANCEL,             IDH_CANCEL,
                                     IDOK,                 IDH_OK,
									 0, 					0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		/* -------------- Cursor characteristics ------------- */

		emudlgInitCursorSettings(hDlg, pstEmuSettings, IDC_UNDERLINE_CURSOR,
			IDC_BLOCK_CURSOR, IDC_BLINK_CURSOR);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

			/* -------------- Cursor type ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_BLOCK_CURSOR) ?
					EMU_CURSOR_BLOCK : EMU_CURSOR_LINE;

			/* -------------- Cursor Blink ------------- */

			pstEmuSettings->fCursorBlink =
				(int)IsDlgButtonChecked(hDlg, IDC_BLINK_CURSOR);

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuViewdata_SettingsDlgProc
 *
 * DESCRIPTION:
 *	TTY Settings dialog proc.
 *
 * ARGUMENTS:
 *	Standard window proc parameters.
 *
 * RETURNS:
 *	Standerd return value.
 *
 */
INT_PTR CALLBACK emuViewdata_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	PSTEMUSET	pstEmuSettings;
	static 		DWORD aHlpTable[] = {IDC_DESTRUCTIVE_BKSP,  IDH_TERM_EMUSET_DESTRUCTIVE,
									 IDC_BLOCK_CURSOR,	    IDH_TERM_EMUSET_CURSOR,
									 IDC_UNDERLINE_CURSOR,  IDH_TERM_EMUSET_CURSOR,
									 IDC_BLINK_CURSOR,	    IDH_TERM_EMUSET_CURSOR,
									 IDC_GR_CURSOR, 	    IDH_TERM_EMUSET_CURSOR,
							         IDC_SEND_POUND_SYMBOL, IDH_TERM_SETTING_ENTER,
                                     IDC_HIDE_CURSOR,       IDH_TERM_SETTING_HIDE_CURSOR,
                                     IDCANCEL,              IDH_CANCEL,
                                     IDOK,                  IDH_OK,
		                             0,0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pstEmuSettings = (PSTEMUSET)lPar;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pstEmuSettings);
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
		/* -------------- Screen rows and columns ------------- */

		emudlgInitRowsCols(hDlg, pstEmuSettings);
#endif
		/* -------------- Hide cursor ------------- */

		SendDlgItemMessage(hDlg, IDC_HIDE_CURSOR, BM_SETCHECK,
			(pstEmuSettings->nCursorType == EMU_CURSOR_NONE) ? 1 : 0,
			0);

		/* -------------- Enter key sends # ------------- */

		SendDlgItemMessage(hDlg, IDC_SEND_POUND_SYMBOL, BM_SETCHECK,
			(pstEmuSettings->fLbSymbolOnEnter == TRUE) ? 1 : 0, 0);

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
			pstEmuSettings = (PSTEMUSET)GetWindowLongPtr(hDlg, GWLP_USERDATA);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
			/* -------------- Screen rows and columns ------------- */

			emudlgGetRowColSettings(hDlg, pstEmuSettings);
#endif
			/* -------------- Hide cursor ------------- */

			pstEmuSettings->nCursorType =
				(int)IsDlgButtonChecked(hDlg, IDC_HIDE_CURSOR) ?
					EMU_CURSOR_NONE : EMU_CURSOR_LINE;
			
			/* -------------- Enter key sends # ------------- */

			pstEmuSettings->fLbSymbolOnEnter =
				(int)IsDlgButtonChecked(hDlg, IDC_SEND_POUND_SYMBOL) ?
					TRUE : FALSE;

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emudlgInitCursorSettings
 *
 * DESCRIPTION:
 *	Initialize cursor settings.
 *
 * ARGUMENTS:
 * 	hDlg - dialog window.
 *	pstEmuSettings 	- pointer to the emulator settings structure.
 *
 * RETURNS:
 *	void.
 *
 */
STATIC_FUNC void emudlgInitCursorSettings(HWND  hDlg,
									      PSTEMUSET pstEmuSettings,
									      INT  ID_UNDERLINE,
									      INT  ID_BLOCK,
									      INT  ID_BLINK)
	{
	int i;

	switch (pstEmuSettings->nCursorType)
		{
	case EMU_CURSOR_LINE:   i = ID_UNDERLINE;	break;
	case EMU_CURSOR_BLOCK: 	i = ID_BLOCK;		break;
	default:				i = ID_UNDERLINE;	break;
		}

	SendDlgItemMessage(hDlg, i, BM_SETCHECK, 1, 0);

	SendDlgItemMessage(hDlg, ID_BLINK, BM_SETCHECK,
		(unsigned int)pstEmuSettings->fCursorBlink, 0);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emudlgInitCharSetSetting
 *
 * DESCRIPTION:
 *	Initialize the character set setting.
 *
 * ARGUMENTS:
 * 	hDlg - dialog window.
 *	pstEmuSettings 	- pointer to the emulator settings structure.
 *
 * RETURNS:
 *	void.
 *
 * AUTHOR: Bob Everett - 3 Jun 98
 */
STATIC_FUNC void emudlgInitCharSetSetting(HWND  hDlg,
											PSTEMUSET pstEmuSettings,
											int nCharSetTableID,
											int nDefaultCharSetID)
	{
	BOOL	fResult = TRUE;
	int		nLen, nIndex;
	BYTE   *pb, *pbSel;

	if (resLoadDataBlock(glblQueryDllHinst(), nCharSetTableID,
			(LPVOID *)&pb, &nLen))
		{
		assert(FALSE);
		}
	else
		{
		if (!emudlgFindCharSetName(hDlg, pb, pstEmuSettings->nCharacterSet,
				(LPWSTR *)&pbSel, TRUE))
			{
			// Couldn't find the current character set in the table of
			// characters sets. This happens when switching from one
			// terminal type to another that doesn't contain the char
			// set.	Use the default character set.
			pstEmuSettings->nCharacterSet = nDefaultCharSetID;
			if (!emudlgFindCharSetName(hDlg, pb, pstEmuSettings->nCharacterSet,
					(LPWSTR *)&pbSel, FALSE))
				{
				// We've got problems.
				fResult = FALSE;
				assert(FALSE);
				}
			}

		if (fResult)
			{
			//JMH 01-09-97 Now select the string corresponding to
			// nCharacterSet.
	        nIndex = (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET,
					CB_SELECTSTRING, 0, (LPARAM)(LPWSTR)pbSel);
			assert(nIndex != CB_ERR);
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emudlgFindCharSetName
 *
 * DESCRIPTION:
 *	Finds the appropriate character set settings.
 *
 * ARGUMENTS:
 *	hDlg - dialog window handle
 * 	pbCharSetTable - address of the emu's table of character sets
 *	pszCharSetName - address at which to put the char set name
 *	fTellDlg - TRUE if the dialog should be made aware of the table
 *
 * RETURNS:
 *	TRUE if successful, FALSE if not.
 *
 * AUTHOR: Bob Everett - 3 Jun 98
 */
STATIC_FUNC BOOL emudlgFindCharSetName(HWND  hDlg,
										BYTE *pbCharSetTable,
										int nCharSetID,
										LPWSTR *ppszCharSetName,
										BOOL fTellDlg)
	{
	BOOL	fRetVal = FALSE;
	int		nCnt, nLen, nEmuCount, nIndex, nCharSet;
	BYTE	*pb = pbCharSetTable;

	nEmuCount = *(RCDATA_TYPE *)pb;
	pb += sizeof(RCDATA_TYPE);

	for (nCnt = 0 ; nCnt < nEmuCount; nCnt++)
		{
		nLen = StrCharGetByteCount((LPWSTR)pb) + (int)sizeof(BYTE);

		if (fTellDlg)
			{
			nIndex = (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_ADDSTRING, 0,
					(LPARAM)(LPWSTR)pb);
			assert(nIndex != CB_ERR);
			}

		#if FALSE	// DEADWOOD:rde 10 Mar 98
        //JMH 01-09-97 Because this list gets sorted, we have to store the
        // table index with each entry, or else the selection index we get
        // when OK is pressed won't mean much.
        //
        nIndex = SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_SETITEMDATA,
            (WPARAM) nIndex, (LPARAM) i);
        assert(nIndex != CB_ERR);
		#endif
		// Save the real char set id with the string. rde 10 Mar 98
		nCharSet = *((RCDATA_TYPE *)(pb + nLen));

		if (fTellDlg)
			{
			nIndex = (int)SendDlgItemMessage(hDlg, IDC_CHARACTER_SET, CB_SETITEMDATA,
					(WPARAM)nIndex, (LPARAM)nCharSet);
			assert(nIndex != CB_ERR);
			}

		// Must match the char set id to nCharacterSet, not the order in
		// which they're listed in the resource data block. rde 10 Mar 98
		//if (i == pstEmuSettings->nCharacterSet)
		if (nCharSet == nCharSetID)
            {
            //JMH 01-09-97 Store a pointer to the string corresponding to
            // nCharacterSet, so we can select the appropriate entry after
            // they've all been sorted.
            //
            *ppszCharSetName = (LPWSTR)pb;
			fRetVal = TRUE;
            }

		pb += (nLen + (int)sizeof(RCDATA_TYPE));
		}

	return fRetVal;
	}

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
STATIC_FUNC void emudlgInitColorThemeCombo(HWND hDlg,
		PSTEMUSET pstEmuSettings)
	{
	int i;
	int nIndex;
	HWND hwndCombo;

	hwndCombo = GetDlgItem(hDlg, IDC_COLOR_THEME_COMBO);
	if (hwndCombo == 0 || pstEmuSettings == NULL)
		return;

	if (pstEmuSettings->nColorTheme < 0 ||
			pstEmuSettings->nColorTheme >= EMU_COLOR_THEME_COUNT)
		pstEmuSettings->nColorTheme = EMU_COLOR_THEME_SOLARIZED_LIGHT;

	for (i = 0; i < EMU_COLOR_THEME_COUNT; ++i)
		{
		nIndex = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0,
				(LPARAM)termColorThemeName(i));
		if (nIndex != CB_ERR)
			{
			SendMessage(hwndCombo, CB_SETITEMDATA, (WPARAM)nIndex, i);
			if (i == pstEmuSettings->nColorTheme)
				SendMessage(hwndCombo, CB_SETCURSEL, (WPARAM)nIndex, 0);
			}
		}
	}

STATIC_FUNC int emudlgColorFromControlId(int nId, BOOL *pfText)
	{
	typedef struct stColorControl
		{
		int nTextId;
		int nBackId;
		int nColor;
		} STCOLORCONTROL;

	static const STCOLORCONTROL astControls[] =
		{
		{ IDC_TEXT_BLACK,       IDC_BACK_BLACK,       VC_BLACK },
		{ IDC_TEXT_DARKBLUE,    IDC_BACK_DARKBLUE,    VC_BLUE },
		{ IDC_TEXT_DARKGREEN,   IDC_BACK_DARKGREEN,   VC_GREEN },
		{ IDC_TEXT_DARKCYAN,    IDC_BACK_DARKCYAN,    VC_CYAN },
		{ IDC_TEXT_DARKRED,     IDC_BACK_DARKRED,     VC_RED },
		{ IDC_TEXT_DARKMAGENTA, IDC_BACK_DARKMAGENTA, VC_MAGENTA },
		{ IDC_TEXT_DARKYELLOW,  IDC_BACK_DARKYELLOW,  VC_BROWN },
		{ IDC_TEXT_LIGHTGRAY,   IDC_BACK_LIGHTGRAY,   VC_WHITE },
		{ IDC_TEXT_DARKGRAY,    IDC_BACK_DARKGRAY,    VC_GRAY },
		{ IDC_TEXT_BLUE,        IDC_BACK_BLUE,        VC_BRT_BLUE },
		{ IDC_TEXT_GREEN,       IDC_BACK_GREEN,       VC_BRT_GREEN },
		{ IDC_TEXT_CYAN,        IDC_BACK_CYAN,        VC_BRT_CYAN },
		{ IDC_TEXT_RED,         IDC_BACK_RED,         VC_BRT_RED },
		{ IDC_TEXT_MAGENTA,     IDC_BACK_MAGENTA,     VC_BRT_MAGENTA },
		{ IDC_TEXT_YELLOW,      IDC_BACK_YELLOW,      VC_BRT_YELLOW },
		{ IDC_TEXT_WHITE,       IDC_BACK_WHITE,       VC_BRT_WHITE }
		};
	int i;

	for (i = 0; i < DIM(astControls); ++i)
		{
		if (nId == astControls[i].nTextId)
			{
			if (pfText)
				*pfText = TRUE;
			return astControls[i].nColor;
			}
		if (nId == astControls[i].nBackId)
			{
			if (pfText)
				*pfText = FALSE;
			return astControls[i].nColor;
			}
		}

	return -1;
	}

STATIC_FUNC void emudlgUpdateColorPreview(HWND hDlg)
	{
	STCOLORDLG *pstDlg;

	pstDlg = (STCOLORDLG *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if (pstDlg && pstDlg->hPreviewBrush)
		{
		DeleteObject(pstDlg->hPreviewBrush);
		pstDlg->hPreviewBrush = NULL;
		}

	InvalidateRect(GetDlgItem(hDlg, IDC_TEXT_PREVIEW), NULL, TRUE);
	InvalidateRect(GetDlgItem(hDlg, IDC_TEXT_BLACK), NULL, TRUE);
	InvalidateRect(GetDlgItem(hDlg, IDC_BACK_BLACK), NULL, TRUE);
	InvalidateRect(hDlg, NULL, FALSE);
	}

STATIC_FUNC BOOL emudlgDrawColorButton(HWND hDlg,
		const DRAWITEMSTRUCT *pdis)
	{
	STCOLORDLG *pstDlg;
	PSTEMUSET pstEmuSettings;
	BOOL fText;
	int nColor;
	COLORREF cr;
	HBRUSH hBrush;
	RECT rc;
	BOOL fSelected;

	if (pdis == NULL)
		return FALSE;

	nColor = emudlgColorFromControlId((int)pdis->CtlID, &fText);
	if (nColor < 0)
		return FALSE;

	pstDlg = (STCOLORDLG *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if (pstDlg == NULL || pstDlg->pstEmuSettings == NULL)
		return FALSE;
	pstEmuSettings = pstDlg->pstEmuSettings;

	rc = pdis->rcItem;
	cr = termColorThemeColor(pstEmuSettings->nColorTheme, nColor);
	hBrush = CreateSolidBrush(cr);
	if (hBrush)
		{
		FillRect(pdis->hDC, &rc, hBrush);
		DeleteObject(hBrush);
		}

	fSelected = fText ?
			(pstEmuSettings->nTextColor == nColor) :
			(pstEmuSettings->nBackgroundColor == nColor);
	if (fSelected)
		{
		InflateRect(&rc, -1, -1);
		DrawEdge(pdis->hDC, &rc, EDGE_SUNKEN, BF_RECT);
		}
	else
		{
		DrawEdge(pdis->hDC, &rc, EDGE_RAISED, BF_RECT);
		}

	return TRUE;
	}
#endif

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emudlgInitRowsCols
 *
 * DESCRIPTION:
 *  Sets up the row and column fields.
 *
 * ARGUMENTS:
 *  hDlg - edit control window.
 *	pstEmuSettings - address of emulator settings structure.
 *
 * RETURNS:
 *  void.
 *
 * AUTHOR: Bob Everett - 22 Jun 1998
 */
STATIC_FUNC void emudlgInitRowsCols(HWND hDlg, PSTEMUSET pstEmuSettings)
	{
	WCHAR		achString[20];
	WCHAR		achFormat[20];

	SendDlgItemMessage(hDlg, IDC_NUMBER_OF_ROWS, EM_LIMITTEXT, 2, 0);

	LoadString(glblQueryDllHinst(), IDS_XD_INT, achFormat,
			sizeof(achFormat) / sizeof(WCHAR));
	WCHAR_Fill(achString, L'\0', sizeof(achString) / sizeof(WCHAR));
	wsprintf(achString, achFormat, pstEmuSettings->nUserDefRows);
	SendDlgItemMessage(hDlg, IDC_NUMBER_OF_ROWS, WM_SETTEXT, 0,
			(LPARAM)(LPWSTR)achString);

	SendDlgItemMessage(hDlg, IDC_NUMBER_OF_COLS, EM_LIMITTEXT, 3, 0);

	LoadString(glblQueryDllHinst(), IDS_XD_INT, achFormat,
			sizeof(achFormat) / sizeof(WCHAR));
	WCHAR_Fill(achString, L'\0', sizeof(achString) / sizeof(WCHAR));
	wsprintf(achString, achFormat, pstEmuSettings->nUserDefCols);
	SendDlgItemMessage(hDlg, IDC_NUMBER_OF_COLS, WM_SETTEXT, 0,
			(LPARAM)(LPWSTR)achString);

	// Put the spin buttons on the row and column fields.
	emudlgCreateUpDownControl(hDlg, pstEmuSettings);
	}
#endif

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emudlgCreateUpDownControl
 *
 * DESCRIPTION:
 *  Gets the final row and column settings.
 *
 * ARGUMENTS:
 *  hDlg - edit control window.
 *	pstEmuSettings - address of emulator settings structure.
 *
 * RETURNS:
 *  void.
 *
 * AUTHOR: Bob Everett - 22 Jun 1998
 */
STATIC_FUNC void emudlgGetRowColSettings(HWND hDlg, PSTEMUSET pstEmuSettings)
	{
	pstEmuSettings->nUserDefRows = emudlgValidateEntryFieldSetting(hDlg,
			IDC_NUMBER_OF_ROWS, MIN_EMUROWS, MAX_EMUROWS);

	pstEmuSettings->nUserDefCols = emudlgValidateEntryFieldSetting(hDlg,
			IDC_NUMBER_OF_COLS, MIN_EMUCOLS, MAX_EMUCOLS);

	pstEmuSettings->f132Columns =
			(pstEmuSettings->nUserDefCols == 132 ? TRUE : FALSE);
	}
#endif

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emudlgCreateUpDownControl
 *
 * DESCRIPTION:
 *  This function puts an up-down control on the edit field for the row and
 *	column fields. This gives us bounds checking for free... just set the
 *	appropriate parameters in the CreateUpDownControl call.
 *
 *	NOTE: This is a duplicate of CreateUpDownControl
 *
 * ARGUMENTS:
 *  hDlg - edit control window.
 *
 * RETURNS:
 *  void.
 *
 * AUTHOR: Bob Everett - 8 Jun 1998
 */
STATIC_FUNC void emudlgCreateUpDownControl(HWND hDlg, PSTEMUSET pstEmuSettings)
	{
	RECT	rc;
	int		nHeight, nWidth;
	DWORD	dwFlags;
	HWND	hwndChild;

    // Draw a spin control for the rows field.
    GetClientRect(GetDlgItem(hDlg, IDC_NUMBER_OF_ROWS), &rc);
	nHeight = rc.top - rc.bottom;
	nWidth = (nHeight / 3) * 2;

	dwFlags = WS_CHILD       | WS_VISIBLE |
			  UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT;

	hwndChild = CreateUpDownControl(
					dwFlags,			// create window flags
					rc.right,			// left edge
					rc.top,				// top edge
					nWidth,				// width
					nHeight,			// height
					hDlg,				// parent window
					IDC_EDIT_ROWS,
					(HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE),
					GetDlgItem(hDlg, IDC_NUMBER_OF_ROWS),
					MAX_EMUROWS,
					MIN_EMUROWS,
					pstEmuSettings->nUserDefRows);

    // Repeat for the columns field.
    GetClientRect(GetDlgItem(hDlg, IDC_NUMBER_OF_COLS), &rc);
	nHeight = rc.top - rc.bottom;
	nWidth = (nHeight / 3) * 2;

	hwndChild = CreateUpDownControl(
					dwFlags,			// create window flags
					rc.right,			// left edge
					rc.top,				// top edge
					nWidth,				// width
					nHeight,			// height
					hDlg,				// parent window
					IDC_EDIT_COLUMNS,
					(HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE),
					GetDlgItem(hDlg, IDC_NUMBER_OF_COLS),
					MAX_EMUCOLS,
					MIN_EMUCOLS,
					pstEmuSettings->nUserDefCols);    			

    assert(hwndChild);
	}
#endif

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emudlgValidateEntryFieldSetting
 *
 * DESCRIPTION:
 *  If the user entered a value outside of the range we support force the
 *	value into the range.
 *
 *	Note: copied from propValidateBackscrlSize.
 *
 * ARGUMENTS:
 *  hDlg - dialog window handle.
 *
 * RETURNS:
 *  nNewValue - number of lines to keep in the backscrol buffer.
 *
 * AUTHOR: Bob Everett - 8 Jun 1998
 */
STATIC_FUNC int emudlgValidateEntryFieldSetting(HWND hDlg,
										int nIDC,
										int nMinVal,
										int nMaxVal)
	{
	int		nValue = 0, nNewValue = 0;
	WCHAR	achStrEntered[20], achFormat[20];

	WCHAR_Fill(achStrEntered, L'\0', sizeof(achStrEntered) / sizeof(WCHAR));
	GetDlgItemText(hDlg, nIDC, achStrEntered, sizeof(achStrEntered));

	nNewValue = nValue = (int)wcstol(achStrEntered, NULL, 10);
	if (nValue > nMaxVal)
		nNewValue = nMaxVal;
	else if (nValue < nMinVal)
		nNewValue = nMinVal;

	if (nNewValue != nValue)
		{
		LoadString(glblQueryDllHinst(),
					IDS_XD_INT,
					achFormat,
					sizeof(achFormat) / sizeof(WCHAR));
		WCHAR_Fill(achStrEntered,
					L'\0',
					sizeof(achStrEntered) / sizeof(WCHAR));
		wsprintf(achStrEntered, achFormat, nNewValue);
		SendDlgItemMessage(hDlg,
					nIDC,
					WM_SETTEXT,
					0,
					(LPARAM)(LPWSTR)achStrEntered);
		}

	return (nNewValue);
	}
#endif
