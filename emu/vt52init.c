/*	File: D:\WACKER\emu\vt52init.c (Created: 28-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/02/01 3:59p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll/stdtyp.h>
#include <tdll/session.h>
#include <tdll/assert.h>
#include <tdll/mc.h>
#include <tdll/backscrl.h>
#include <tdll/ut_text.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "emudec.hh"
#include "keytbls.h"

static void vt52char_reset(const HHEMU hhEmu);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52_init
 *
 * DESCRIPTION:
 *	 Loads and initializes the VT52 emulator.
 *
 * ARGUMENTS:
 *	 new_emu -- TRUE if emulating a power up on the real thing.
 *
 * RETURNS:
 *	 nothing
 */
void vt52_init(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI;

	static struct trans_entry const vt52_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
		{0, ECHAR_CAST('\x20'),	ECHAR_CAST('\x7E'),	emuStdGraphic}, 	// Space - ~
		{1, ECHAR_CAST('\x1B'),	ECHAR_CAST('\x1B'),	nothing},			// Esc
		{0, ECHAR_CAST('\x07'),	ECHAR_CAST('\x07'),	emu_bell},			// Ctrl-G
		{0, ECHAR_CAST('\x08'),	ECHAR_CAST('\x08'),	vt_backspace},		// Backspace
		{0, ECHAR_CAST('\x09'),	ECHAR_CAST('\x09'),	tabn},				// Tab
		{0, ECHAR_CAST('\x0A'),	ECHAR_CAST('\x0A'),	emuLineFeed},		// New Line
		{0, ECHAR_CAST('\x0D'),	ECHAR_CAST('\x0D'),	carriagereturn},	// CR
		{5, ECHAR_CAST('\x18'),	ECHAR_CAST('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X

		{NEW_STATE, 0, 0, 0}, // State 1						// Esc
		{2, ECHAR_CAST('\x59'),	ECHAR_CAST('\x59'),	nothing},			// Y
		{0, ECHAR_CAST('\x3C'),	ECHAR_CAST('\x3C'),	vt52_toANSI},		// <
		{0, ECHAR_CAST('\x3D'),	ECHAR_CAST('\x3E'),	vt_alt_kpmode}, 	// = - >
		{0, ECHAR_CAST('\x41'),	ECHAR_CAST('\x41'),	ANSI_CUU},			// A
		{0, ECHAR_CAST('\x42'),	ECHAR_CAST('\x42'),	ANSI_CUD},			// B
		{0, ECHAR_CAST('\x43'),	ECHAR_CAST('\x43'),	ANSI_CUF},			// C
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	vt_CUB},			// D
		{0, ECHAR_CAST('\x46'),	ECHAR_CAST('\x47'),	vt_charshift},		// F - G
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	ANSI_CUP},			// H
		{0, ECHAR_CAST('\x49'),	ECHAR_CAST('\x49'),	ANSI_RI},			// I
		{0, ECHAR_CAST('\x4A'),	ECHAR_CAST('\x4A'),	ANSI_ED},			// J
		{0, ECHAR_CAST('\x4B'),	ECHAR_CAST('\x4B'),	ANSI_EL},			// K
		{0, ECHAR_CAST('\x56'),	ECHAR_CAST('\x56'),	vt52PrintCommands}, // V
		{4, ECHAR_CAST('\x57'),	ECHAR_CAST('\x57'),	nothing},			// W
		{0, ECHAR_CAST('\x58'),	ECHAR_CAST('\x58'),	nothing},			// X
		{0, ECHAR_CAST('\x5A'),	ECHAR_CAST('\x5A'),	vt52_id},			// Z
		{0, ECHAR_CAST('\x5D'),	ECHAR_CAST('\x5D'),	vt52PrintCommands}, // ]
		{0, ECHAR_CAST('\x5E'),	ECHAR_CAST('\x5E'),	vt52PrintCommands}, // ^
		{0, ECHAR_CAST('\x5F'),	ECHAR_CAST('\x5F'),	vt52PrintCommands}, // _

		{NEW_STATE, 0, 0, 0}, // State 2						// EscY
		// Accept all data--CUP will set the limits. Needed for more than 24 rows.
		{3, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	char_pn},			// Space - 8
//		{3, ECHAR_CAST('\x20'),	ECHAR_CAST('\x38'),	char_pn},			// Space - 8

		{NEW_STATE, 0, 0, 0}, // State 3						// EscYn
		// Accept all data--CUP will set the limits. Needed for more than 80 columns.
		{0, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	vt52_CUP},			// Space - o
//		{0, ECHAR_CAST('\x20'),	ECHAR_CAST('\x6F'),	vt52_CUP},			// Space - o

		{NEW_STATE, 0, 0, 0}, // State 4						// EscW
		{4, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	vt52Print}, 		// All

		{NEW_STATE, 0, 0, 0}, // State 5						// Ctrl-X
		{5, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	EmuStdChkZmdm}, 	// All

		};

	emuInstallStateTable(hhEmu, vt52_tbl, DIM(vt52_tbl));

	// Allocate space for and initialize data that is used only by the
	// VT52 emulator.
	//
	if (hhEmu->pvPrivate != 0)
		{
		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	hhEmu->pvPrivate = malloc(sizeof(DECPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
	CnvrtHostToECHAR(pstPRI->terminate, sizeof(pstPRI->terminate), L"\033X",
                     StrCharGetByteCount(L"\033X"));
	pstPRI->len_t = 2;
	pstPRI->pntr = pstPRI->storage;
	pstPRI->len_s = 0;

	// Initialize standard hhEmu values.
	//
	hhEmu->emu_kbdin 	= vt52_kbdin;
	hhEmu->emu_highchar = 0x7E;
	hhEmu->emu_deinstall = emuVT52Unload;	
	
	vt52char_reset(hhEmu);

//	emuKeyTableLoad(hhEmu, IDT_VT52_KEYS, &hhEmu->stEmuKeyTbl);
//	emuKeyTableLoad(hhEmu, IDT_VT52_KEYPAD_APP_MODE, &hhEmu->stEmuKeyTbl2);
//	emuKeyTableLoad(hhEmu, IDT_VT_MAP_PF_KEYS, &hhEmu->stEmuKeyTbl3);
	emuKeyTableLoad(hhEmu, VT52KeyTable, 
					 sizeof(VT52KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl);
	emuKeyTableLoad(hhEmu, VT52_Keypad_KeyTable, 
					 sizeof(VT52_Keypad_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl2);
	emuKeyTableLoad(hhEmu, VT_PF_KeyTable, 
					 sizeof(VT_PF_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl3);

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52char_reset
 *
 * DESCRIPTION:
 *	 Sets the VT52 emulator character set to its RESET conditions.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
static void vt52char_reset(const HHEMU hhEmu)
	{
	// Set up US ASCII character set as G0 and DEC graphics as G1
	//
	vt_charset_init(hhEmu);
	hhEmu->emu_code = ECHAR_CAST(')');
	vt_scs1(hhEmu);
	hhEmu->emu_code = (ECHAR)0;
	vt_scs2(hhEmu);
	}

/* end of vt52init.c */
