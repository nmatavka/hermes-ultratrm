/*	File: D:\WACKER\emu\ansiinit.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll/stdtyp.h>
#include <tdll/session.h>
#include <tdll/cloop.h>
#include <tdll/mc.h>
#include <tdll/assert.h>
#include <tdll/backscrl.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "ansi.hh"
#include "keytbls.h"


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuAnsiInit
 *
 * DESCRIPTION:
 *	 Sets up and installs the ANSI state table. Defines the ANSI
 *	 keyboard. Either resets the emulator completely or redefines emulator
 *	 conditions as they were when last saved.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void emuAnsiInit(const HHEMU hhEmu)
	{
	PSTANSIPRIVATE pstPRI;

	static struct trans_entry const ansi_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
#if 1
		{0, ECHAR_CAST('\x20'),	ECHAR_CAST('\xFF'),	emuStdGraphic}, 	// Space - All
#else
		{0, ECHAR_CAST('\x20'),	0xFFFF,			emuStdGraphic}, 	// Space - All
#endif
		{1, ECHAR_CAST('\x1B'),	ECHAR_CAST('\x1B'),	nothing},			// Esc
		{0, ECHAR_CAST('\x05'),	ECHAR_CAST('\x05'),	vt100_answerback},	// Ctrl-E
		{0, ECHAR_CAST('\x07'),	ECHAR_CAST('\x07'),	emu_bell},			// Ctrl-G
		{0, ECHAR_CAST('\x08'),	ECHAR_CAST('\x08'),	backspace}, 		// Backspace
		{0, ECHAR_CAST('\x09'),	ECHAR_CAST('\x09'),	tabn},				// Tab
		{0, ECHAR_CAST('\x0A'),	ECHAR_CAST('\x0B'),	emuLineFeed},		// NL - VT
		{0, ECHAR_CAST('\x0C'),	ECHAR_CAST('\x0C'),	AnsiFormFeed},		// Form Feed
		{0, ECHAR_CAST('\x0D'),	ECHAR_CAST('\x0D'),	carriagereturn},	// CR
		{3, ECHAR_CAST('\x18'),	ECHAR_CAST('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X
		{0, ECHAR_CAST('\x00'),	ECHAR_CAST('\x1F'),	emuStdGraphic}, 	// All Ctrl's

		{NEW_STATE, 0, 0, 0}, // State 1						// Esc
		{2, ECHAR_CAST('\x5B'),	ECHAR_CAST('\x5B'),	ANSI_Pn_Clr},		// [
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	ANSI_IND},			// D
		{0, ECHAR_CAST('\x45'),	ECHAR_CAST('\x45'),	ANSI_NEL},			// E
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	ANSI_HTS},			// H
		{0, ECHAR_CAST('\x4D'),	ECHAR_CAST('\x4D'),	ANSI_RI},			// M

		{NEW_STATE, 0, 0, 0}, // State 2						// Esc[
		{2, ECHAR_CAST('\x30'),	ECHAR_CAST('\x39'),	ANSI_Pn},			// 0 - 9
		{2, ECHAR_CAST('\x3B'),	ECHAR_CAST('\x3B'),	ANSI_Pn_End},		// ;
		{5, ECHAR_CAST('\x3D'),	ECHAR_CAST('\x3D'),	nothing},			// =
		{2, ECHAR_CAST('\x3A'),	ECHAR_CAST('\x3F'),	ANSI_Pn},			// : - ?
		{0, ECHAR_CAST('\x41'),	ECHAR_CAST('\x41'),	ANSI_CUU},			// A
		{0, ECHAR_CAST('\x42'),	ECHAR_CAST('\x42'),	ANSI_CUD},			// B
		{0, ECHAR_CAST('\x43'),	ECHAR_CAST('\x43'),	ANSI_CUF},			// C
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	ANSI_CUB},			// D
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	ANSI_CUP},			// H
		{0, ECHAR_CAST('\x4A'),	ECHAR_CAST('\x4A'),	ANSI_ED},			// J
		{0, ECHAR_CAST('\x4B'),	ECHAR_CAST('\x4B'),	ANSI_EL},			// K
		{0, ECHAR_CAST('\x4C'),	ECHAR_CAST('\x4C'),	ANSI_IL},			// L
		{0, ECHAR_CAST('\x4D'),	ECHAR_CAST('\x4D'),	ANSI_DL},			// M
		{0, ECHAR_CAST('\x50'),	ECHAR_CAST('\x50'),	ANSI_DCH},			// P
		{0, ECHAR_CAST('\x66'),	ECHAR_CAST('\x66'),	ANSI_CUP},			// f
		{0, ECHAR_CAST('\x67'),	ECHAR_CAST('\x67'),	ANSI_TBC},			// g
		{0, ECHAR_CAST('\x68'),	ECHAR_CAST('\x68'),	ansi_setmode},		// h
		{0, ECHAR_CAST('\x69'),	ECHAR_CAST('\x69'),	vt100PrintCommands},// i
		{0, ECHAR_CAST('\x6C'),	ECHAR_CAST('\x6C'),	ansi_resetmode},	// l
		{0, ECHAR_CAST('\x6D'),	ECHAR_CAST('\x6D'),	ANSI_SGR},			// m
		{0, ECHAR_CAST('\x6E'),	ECHAR_CAST('\x6E'),	ANSI_DSR},			// n
		{0, ECHAR_CAST('\x70'),	ECHAR_CAST('\x70'),	nothing},			// p
		{0, ECHAR_CAST('\x72'),	ECHAR_CAST('\x72'),	vt_scrollrgn},		// r
		{0, ECHAR_CAST('\x73'),	ECHAR_CAST('\x73'),	ansi_savecursor},	// s
		{0, ECHAR_CAST('\x75'),	ECHAR_CAST('\x75'),	ansi_savecursor},	// u

		{NEW_STATE, 0, 0, 0}, // State 3						// Ctrl-X
		{3, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	EmuStdChkZmdm}, 	// all codes

		{NEW_STATE, 0, 0, 0}, // State 4						// Ctrl-A
		{4, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	nothing},			// all codes

		{NEW_STATE, 0, 0, 0}, // State 5						// Esc[=
		{5, ECHAR_CAST('\x32'),	ECHAR_CAST('\x32'),	ANSI_Pn},			// 2
		{5, ECHAR_CAST('\x35'),	ECHAR_CAST('\x35'),	ANSI_Pn},			// 5
		{0, ECHAR_CAST('\x68'),	ECHAR_CAST('\x68'),	DoorwayMode},		// h
		{0, ECHAR_CAST('\x6C'),	ECHAR_CAST('\x6C'),	DoorwayMode},		// l
		};

	emuInstallStateTable(hhEmu, ansi_tbl, DIM(ansi_tbl));

	// Allocate space for and initialize data that is used only by the
	// ANSI emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(ANSIPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTANSIPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(ANSIPRIVATE));

	// Initialize standard handle items.
	//
	hhEmu->emuResetTerminal = emuAnsiReset;

	emuKeyTableLoad(hhEmu, AnsiKeyTable, 
					 sizeof(AnsiKeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl);
	emuKeyTableLoad(hhEmu, IBMPCKeyTable, 
					 sizeof(IBMPCKeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl2);

	hhEmu->emu_kbdin = ansi_kbdin;
	hhEmu->emu_deinstall = emuAnsiUnload;
	emuAnsiReset(hhEmu, FALSE);

#if 1
	hhEmu->emu_highchar = (WCHAR)0xFF;
#else
	hhEmu->emu_highchar = (WCHAR)0xFFFF;
#endif

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuAnsiReset
 *
 * DESCRIPTION:
 *	 Resets the ANSI emulator.
 *
 * ARGUMENTS:
 *	fHostRequest	-	TRUE if result of codes from host
 *
 * RETURNS:
 *	 nothing
 */
int emuAnsiReset(const HHEMU hhEmu, const int fHostRequest)
	{
	hhEmu->mode_KAM = hhEmu->mode_IRM = hhEmu->mode_VEM =
	hhEmu->mode_HEM = hhEmu->mode_LNM = hhEmu->mode_DECCKM =
	hhEmu->mode_DECOM  = hhEmu->mode_DECCOLM  = hhEmu->mode_DECPFF =
	hhEmu->mode_DECPEX = hhEmu->mode_DECSCNM =
	hhEmu->mode_25enab = hhEmu->mode_protect =
	hhEmu->mode_block = hhEmu->mode_local = RESET;

	hhEmu->mode_SRM = hhEmu->mode_DECTCEM = SET;

	hhEmu->mode_AWM = hhEmu->stUserSettings.fWrapLines;

	if (fHostRequest)
		{
		ANSI_Pn_Clr(hhEmu);
		ANSI_SGR(hhEmu);
		ANSI_RIS(hhEmu);
		}

	return 0;
	}

/* end of ansiinit.c */
