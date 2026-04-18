/*	File: D:\WACKER\emu\vt100ini.c (Created: 27-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:29p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll/stdtyp.h>
#include <tdll/session.h>
#include <tdll/assert.h>
#include <tdll/mc.h>
#include <tdll/backscrl.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "emudec.hh"
#include "keytbls.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_init
 *
 * DESCRIPTION:
 *	 Initializes the VT100 emulator.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void vt100_init(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI;
	int iRow;

	static struct trans_entry const vt100_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
#if 1
		{0, ECHAR_CAST('\x20'),	ECHAR_CAST('\x7E'),	emuDecGraphic}, 	// Space - ~
#else
		{0, ECHAR_CAST('\x20'),	0xFFFF,			emuDecGraphic}, 	// Space - ~
#endif
		{1, ECHAR_CAST('\x1B'),	ECHAR_CAST('\x1B'),	nothing},			// Esc
		{0, ECHAR_CAST('\x05'),	ECHAR_CAST('\x05'),	vt100_answerback},	// Ctrl-E
		{0, ECHAR_CAST('\x07'),	ECHAR_CAST('\x07'),	emu_bell},			// Ctrl-G
		{0, ECHAR_CAST('\x08'),	ECHAR_CAST('\x08'),	vt_backspace},		// BackSpace
		{0, ECHAR_CAST('\x09'),	ECHAR_CAST('\x09'),	emuDecTab}, 		// Tab
		{0, ECHAR_CAST('\x0A'),	ECHAR_CAST('\x0C'),	emuLineFeed},		// NL - FF
		{0, ECHAR_CAST('\x0D'),	ECHAR_CAST('\x0D'),	carriagereturn},	// CR
		{0, ECHAR_CAST('\x0E'),	ECHAR_CAST('\x0F'),	vt_charshift},		// Ctrl-N
		{7, ECHAR_CAST('\x18'),	ECHAR_CAST('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X

		{NEW_STATE, 0, 0, 0}, // State 1						// Esc
		{2, ECHAR_CAST('\x5B'),	ECHAR_CAST('\x5B'),	ANSI_Pn_Clr},		// [
		{3, ECHAR_CAST('\x23'),	ECHAR_CAST('\x23'),	nothing},			// #
		{4, ECHAR_CAST('\x28'),	ECHAR_CAST('\x29'),	vt_scs1},			// ( - )
		{0, ECHAR_CAST('\x37'),	ECHAR_CAST('\x38'),	vt100_savecursor},	// 7 - 8
		{1, ECHAR_CAST('\x3B'),	ECHAR_CAST('\x3B'),	ANSI_Pn_End},		// ;
		{0, ECHAR_CAST('\x3D'),	ECHAR_CAST('\x3E'),	vt_alt_kpmode}, 	// = - >
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	emuDecIND}, 		// D
		{0, ECHAR_CAST('\x45'),	ECHAR_CAST('\x45'),	ANSI_NEL},			// E
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	ANSI_HTS},			// H
		{0, ECHAR_CAST('\x4D'),	ECHAR_CAST('\x4D'),	emuDecRI},			// M
		{0, ECHAR_CAST('\x5A'),	ECHAR_CAST('\x5A'),	ANSI_DA},			// Z
		{0, ECHAR_CAST('\x63'),	ECHAR_CAST('\x63'),	vt100_hostreset},	// c

		{NEW_STATE, 0, 0, 0}, // State 2						// Esc[
		{2, ECHAR_CAST('\x3B'),	ECHAR_CAST('\x3B'),	ANSI_Pn_End},		// ;
		{2, ECHAR_CAST('\x30'),	ECHAR_CAST('\x3F'),	ANSI_Pn},			// 0 - ?
		{5, ECHAR_CAST('\x22'),	ECHAR_CAST('\x22'),	nothing},			// "
		{0, ECHAR_CAST('\x41'),	ECHAR_CAST('\x41'),	emuDecCUU}, 		// A
		{0, ECHAR_CAST('\x42'),	ECHAR_CAST('\x42'),	emuDecCUD}, 		// B
		{0, ECHAR_CAST('\x43'),	ECHAR_CAST('\x43'),	emuDecCUF}, 		// C
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	emuDecCUB}, 		// D
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	emuDecCUP}, 		// H
		{0, ECHAR_CAST('\x4A'),	ECHAR_CAST('\x4A'),	emuDecED},			// J
		{0, ECHAR_CAST('\x4B'),	ECHAR_CAST('\x4B'),	ANSI_EL},			// K
		{0, ECHAR_CAST('\x4C'),	ECHAR_CAST('\x4C'),	vt_IL}, 			// L
		{0, ECHAR_CAST('\x4D'),	ECHAR_CAST('\x4D'),	vt_DL}, 			// M
		{0, ECHAR_CAST('\x50'),	ECHAR_CAST('\x50'),	vt_DCH},			// P
		{0, ECHAR_CAST('\x63'),	ECHAR_CAST('\x63'),	ANSI_DA},			// c
		{0, ECHAR_CAST('\x66'),	ECHAR_CAST('\x66'),	emuDecCUP}, 		// f
		{0, ECHAR_CAST('\x67'),	ECHAR_CAST('\x67'),	ANSI_TBC},			// g
		{0, ECHAR_CAST('\x68'),	ECHAR_CAST('\x68'),	ANSI_SM},			// h
		{0, ECHAR_CAST('\x69'),	ECHAR_CAST('\x69'),	vt100PrintCommands},// i
		{0, ECHAR_CAST('\x6C'),	ECHAR_CAST('\x6C'),	ANSI_RM},			// l
		{0, ECHAR_CAST('\x6D'),	ECHAR_CAST('\x6D'),	ANSI_SGR},			// m
		{0, ECHAR_CAST('\x6E'),	ECHAR_CAST('\x6E'),	ANSI_DSR},			// n
		{0, ECHAR_CAST('\x71'),	ECHAR_CAST('\x71'),	nothing},			// q
		{0, ECHAR_CAST('\x72'),	ECHAR_CAST('\x72'),	vt_scrollrgn},		// r
		{0, ECHAR_CAST('\x78'),	ECHAR_CAST('\x78'),	vt100_report},		// x

		{NEW_STATE, 0, 0, 0}, // State 3						// Esc#
		{0, ECHAR_CAST('\x33'),	ECHAR_CAST('\x36'),	emuSetDoubleAttr},	// 3 - 6

		{0, ECHAR_CAST('\x38'),	ECHAR_CAST('\x38'),	vt_screen_adjust},	// 8

		{NEW_STATE, 0, 0, 0}, // State 4						// Esc ( - )
		{0, ECHAR_CAST('\x01'),	ECHAR_CAST('\xFF'),	vt_scs2},			// All

		{NEW_STATE, 0, 0, 0}, // State 5						// Esc["
		{0, ECHAR_CAST('\x70'),	ECHAR_CAST('\x70'),	nothing},			// p

		{NEW_STATE, 0, 0, 0}, // State 6						// Printer control
		{6, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	vt100_prnc},		// All

		{NEW_STATE, 0, 0, 0}, // State 7						// Ctrl-X
		{7, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	EmuStdChkZmdm}, 	// All

		};

	emuInstallStateTable(hhEmu, vt100_tbl, DIM(vt100_tbl));

	// Allocate space for and initialize data that is used only by the
	// VT100 emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(DECPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(DECPRIVATE));

	// Allocate an array to hold line attribute values.
	//
	pstPRI->aiLineAttr = malloc(MAX_EMUROWS * sizeof(int) );

	if (pstPRI->aiLineAttr == 0)
		{
		assert(FALSE);
		return;
		}

	for (iRow = 0; iRow < MAX_EMUROWS; iRow++)
		pstPRI->aiLineAttr[iRow] = NO_LINE_ATTR;

	pstPRI->sv_row			= 0;
	pstPRI->sv_col			= 0;
	pstPRI->gn				= 0;
	pstPRI->sv_AWM			= RESET;
	pstPRI->sv_DECOM		= RESET;
	pstPRI->sv_protectmode	= FALSE;
	pstPRI->fAttrsSaved 	= FALSE;
	pstPRI->pntr			= pstPRI->storage;

	// Initialize hhEmu values for VT100.
	//
	hhEmu->emu_kbdin		= vt100_kbdin;
	hhEmu->emuResetTerminal = vt100_reset;
	hhEmu->emu_setcurpos	= emuDecSetCurPos;
	hhEmu->emu_deinstall	= emuVT100Unload;
	hhEmu->emu_clearscreen	= emuDecClearScreen;

#if 1
	hhEmu->emu_highchar 	= 0x7E;
#else
	hhEmu->emu_highchar 	= 0xFFFF;
#endif
	hhEmu->emu_maxcol		= VT_MAXCOL_80MODE;
	hhEmu->mode_vt220		= FALSE;

	std_dsptbl(hhEmu, TRUE);
	vt_charset_init(hhEmu);

	emuKeyTableLoad(hhEmu, VT100KeyTable, 
					 sizeof(VT100KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl);

	emuKeyTableLoad(hhEmu, VT100_Keypad_KeyTable, 
					 sizeof(VT100_Keypad_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl2);

	emuKeyTableLoad(hhEmu, VT100_Cursor_KeyTable, 
					 sizeof(VT100_Cursor_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl3);

	emuKeyTableLoad(hhEmu, VT_PF_KeyTable, 
					 sizeof(VT_PF_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl4);

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

/* end of vt100ini.c */
