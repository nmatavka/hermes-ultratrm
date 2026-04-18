/*	File: D:\WACKER\emu\autoinit.c (Created: 28-Feb-1994)
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
#include <tdll/assert.h>
#include <tdll/mc.h>
#include <tdll/backscrl.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "ansi.hh"
#include "viewdata.hh"
#include "minitel.hh"
#include "keytbls.h"


static void emuAutoNothingVT52(const HHEMU hhEmu);
static void emuAutoVT52toAnsi(const HHEMU hhEmu);
static void emuAutoAnsiEdVT52(const HHEMU hhEmu);
static void emuAutoAnsiElVT52(const HHEMU hhEmu);
static void emuAutoCharPnVT52(const HHEMU hhEmu);
static void emuAutoNothingVT100(const HHEMU hhEmu);
static void emuAutoScs1VT100(const HHEMU hhEmu);
static void emuAutoSaveCursorVT100(const HHEMU hhEmu);
static void emuAutoAnsiPnEndVT100(const HHEMU hhEmu);
static void emuAutoResetVT100(const HHEMU hhEmu);
static void emuAutoAnsiDaVT100(const HHEMU hhEmu);
static void emuAutoReportVT100(const HHEMU hhEmu);
static void emuAutoNothingViewdata(const HHEMU hhEmu);
static void emuAutoSetAttrViewdata(const HHEMU hhEmu);
static void emuAutoNothingAnsi(const HHEMU hhEmu);
static void emuAutoScrollAnsi(const HHEMU hhEmu);
static void emuAutoSaveCurAnsi(const HHEMU hhEmu);
static void emuAutoPnAnsi(const HHEMU hhEmu);
static void emuAutoDoorwayAnsi(const HHEMU hhEmu);
static void emuAutoNothingMinitel(const HHEMU hhEmu);
static void emuAutoMinitelCharAttr(const HHEMU hhEmu);
static void emuAutoMinitelFieldAttr(const HHEMU hhEmu);
static void emuAutoMinitelCursorReport(const HHEMU hhEmu);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuAutoInit
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void emuAutoInit(const HHEMU hhEmu)
	{
	PSTANSIPRIVATE pstPRI;

	static struct trans_entry const astfAutoAnsiTable[] =
		{
		// State 0
		//
		// Ansi emulation occupies all of state 0 codes.
		//
		{NEW_STATE, 0, 0, 0},
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
		//
		// State 1
		// At this point, an ESC has been seen.
		//
		{NEW_STATE, 0, 0, 0},										// Esc
		{2, ECHAR_CAST('\x5B'),	ECHAR_CAST('\x5B'),	ANSI_Pn_Clr},			// [
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	ANSI_IND},				// D
		{0, ECHAR_CAST('\x45'),	ECHAR_CAST('\x45'),	ANSI_NEL},				// E
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	ANSI_HTS},				// H
		{0, ECHAR_CAST('\x4D'),	ECHAR_CAST('\x4D'),	ANSI_RI},				// M
		//
		// Autodetect sequences for for VT52, State 1.
		//
		{2, ECHAR_CAST('\x59'),	ECHAR_CAST('\x59'),	emuAutoNothingVT52},	// Y
		{0, ECHAR_CAST('\x3C'),	ECHAR_CAST('\x3C'),	emuAutoVT52toAnsi}, 	// <
		{0, ECHAR_CAST('\x4A'),	ECHAR_CAST('\x4A'),	emuAutoAnsiEdVT52}, 	// J
		{0, ECHAR_CAST('\x4B'),	ECHAR_CAST('\x4B'),	emuAutoAnsiElVT52}, 	// K
		//
		// Autodetect sequences for for VT100, State 1.
		//
#if !defined(INCL_MINITEL)
		{3, ECHAR_CAST('\x23'),	ECHAR_CAST('\x23'),	emuAutoNothingVT100},	// #
#endif
		{4, ECHAR_CAST('\x28'),	ECHAR_CAST('\x29'),	emuAutoScs1VT100},		// ( - )
		{0, ECHAR_CAST('\x38'),	ECHAR_CAST('\x38'),	emuAutoSaveCursorVT100},// 8
#if !defined(INCL_MINITEL)
		{1, ECHAR_CAST('\x3B'),	ECHAR_CAST('\x3B'),	emuAutoAnsiPnEndVT100}, // ;
#endif
		{0, ECHAR_CAST('\x63'),	ECHAR_CAST('\x63'),	emuAutoResetVT100}, 	// c
		//
		// Autodetect sequences for for Viewdata, State 1.
		//
#if defined(INCL_VIEWDATA)
		{0, ECHAR_CAST('\x31'),	ECHAR_CAST('\x34'),	emuAutoNothingViewdata},// 1 - 4
#endif
		//
		// Autodetect sequences for for Minitel, State 1.
		//
#if defined(INCL_MINITEL)
		//{1, ECHAR_CAST('\x00'),  ECHAR_CAST('\x00'),  emuAutoNothingMinitel},
		//{14,ECHAR_CAST('\x25'),  ECHAR_CAST('\x25'),  emuAutoNothingMinitel},
		//{13,ECHAR_CAST('\x35'),  ECHAR_CAST('\x37'),  emuAutoNothingMinitel},	  // eat ESC,35-37,X sequences
		//{6, ECHAR_CAST('\x39'),  ECHAR_CAST('\x39'),  emuAutoNothingMinitel},	  // PROT1, p134
		{7, ECHAR_CAST('\x3A'),	ECHAR_CAST('\x3A'),	emuAutoNothingMinitel}, 	// PROT2, p134
		{0, ECHAR_CAST('\x40'),	ECHAR_CAST('\x43'),	emuAutoMinitelCharAttr},	// forground color, flashing
		//{0, ECHAR_CAST('\x4C'),  ECHAR_CAST('\x4C'),  emuAutoMinitelCharAttr},	  // char width & height
		//{0, ECHAR_CAST('\x4E'),  ECHAR_CAST('\x4E'),  emuAutoMinitelCharAttr},	  // char width & height
		//{0, ECHAR_CAST('\x4F'),  ECHAR_CAST('\x4F'),  emuAutoMinitelCharAttr},	  // char width & height
		//{0, ECHAR_CAST('\x50'),  ECHAR_CAST('\x59'),  emuAutoMinitelFieldAttr},   // background, underlining
		//{0, ECHAR_CAST('\x5F'),  ECHAR_CAST('\x5F'),  emuAutoMinitelFieldAttr},   // reveal display
		//{0, ECHAR_CAST('\x5C'),  ECHAR_CAST('\x5D'),  emuAutoMinitelCharAttr},	  // inverse
		//{0, ECHAR_CAST('\x61'),  ECHAR_CAST('\x61'),  emuAutoMinitelCursorReport},
#endif
		//
		// State 2
		//
		{NEW_STATE, 0, 0, 0},										// Esc[
		{2, ECHAR_CAST('\x30'),	ECHAR_CAST('\x39'),	ANSI_Pn},				// 0 - 9
		{2, ECHAR_CAST('\x3B'),	ECHAR_CAST('\x3B'),	ANSI_Pn_End},			// ;
		{5, ECHAR_CAST('\x3D'),	ECHAR_CAST('\x3D'),	nothing},				// =
		{2, ECHAR_CAST('\x3A'),	ECHAR_CAST('\x3F'),	ANSI_Pn},				// : - ?
		{0, ECHAR_CAST('\x41'),	ECHAR_CAST('\x41'),	ANSI_CUU},				// A
		{0, ECHAR_CAST('\x42'),	ECHAR_CAST('\x42'),	ANSI_CUD},				// B
		{0, ECHAR_CAST('\x43'),	ECHAR_CAST('\x43'),	ANSI_CUF},				// C
		{0, ECHAR_CAST('\x44'),	ECHAR_CAST('\x44'),	ANSI_CUB},				// D
		{0, ECHAR_CAST('\x48'),	ECHAR_CAST('\x48'),	ANSI_CUP},				// H
		{0, ECHAR_CAST('\x4A'),	ECHAR_CAST('\x4A'),	ANSI_ED},				// J
		{0, ECHAR_CAST('\x4B'),	ECHAR_CAST('\x4B'),	ANSI_EL},				// K
		{0, ECHAR_CAST('\x4C'),	ECHAR_CAST('\x4C'),	ANSI_IL},				// L
		{0, ECHAR_CAST('\x4D'),	ECHAR_CAST('\x4D'),	ANSI_DL},				// M
		{0, ECHAR_CAST('\x50'),	ECHAR_CAST('\x50'),	ANSI_DCH},				// P
		{0, ECHAR_CAST('\x66'),	ECHAR_CAST('\x66'),	ANSI_CUP},				// f
		{0, ECHAR_CAST('\x67'),	ECHAR_CAST('\x67'),	ANSI_TBC},				// g
		{0, ECHAR_CAST('\x68'),	ECHAR_CAST('\x68'),	ansi_setmode},			// h
		{0, ECHAR_CAST('\x69'),	ECHAR_CAST('\x69'),	vt100PrintCommands},	// i
		{0, ECHAR_CAST('\x6C'),	ECHAR_CAST('\x6C'),	ansi_resetmode},		// l
		{0, ECHAR_CAST('\x6D'),	ECHAR_CAST('\x6D'),	ANSI_SGR},				// m
		{0, ECHAR_CAST('\x6E'),	ECHAR_CAST('\x6E'),	ANSI_DSR},				// n
		{0, ECHAR_CAST('\x70'),	ECHAR_CAST('\x70'),	emuAutoNothingAnsi},	// p
		{0, ECHAR_CAST('\x72'),	ECHAR_CAST('\x72'),	emuAutoScrollAnsi}, 	// r
		{0, ECHAR_CAST('\x73'),	ECHAR_CAST('\x73'),	emuAutoSaveCurAnsi},	// s
		{0, ECHAR_CAST('\x75'),	ECHAR_CAST('\x75'),	ansi_savecursor},		// u
		//
		//Autodetect sequences for for VT52, State 2.
		//
		{3, ECHAR_CAST('\x20'),	ECHAR_CAST('\x20'),	emuAutoCharPnVT52}, 	// Space 
		{3, ECHAR_CAST('\x22'),	ECHAR_CAST('\x22'),	emuAutoCharPnVT52}, 	// "
		{3, ECHAR_CAST('\x24'),	ECHAR_CAST('\x2F'),	emuAutoCharPnVT52}, 	// $ - /
		//
		// Autodetect sequences for for VT100, State 2.
		//
		{0, ECHAR_CAST('\x63'),	ECHAR_CAST('\x63'),	emuAutoAnsiDaVT100},	// c
		{0, ECHAR_CAST('\x71'),	ECHAR_CAST('\x71'),	emuAutoNothingVT100},	// q
		{0, ECHAR_CAST('\x78'),	ECHAR_CAST('\x78'),	emuAutoReportVT100},	// x
		//
		// State 3
		//
		{NEW_STATE, 0, 0, 0},
		{3, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	EmuStdChkZmdm}, 		// All
		//
		// State 4
		//
		{NEW_STATE, 0, 0, 0},
		{4, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	nothing},				// All
		//
		// State 5
		//
		{NEW_STATE, 0, 0, 0},
		{5, ECHAR_CAST('\x32'),	ECHAR_CAST('\x32'),	emuAutoPnAnsi}, 		// 2
		{5, ECHAR_CAST('\x35'),	ECHAR_CAST('\x35'),	emuAutoPnAnsi}, 		// 5
		{0, ECHAR_CAST('\x68'),	ECHAR_CAST('\x68'),	emuAutoDoorwayAnsi},	// h
		{0, ECHAR_CAST('\x6C'),	ECHAR_CAST('\x6C'),	emuAutoDoorwayAnsi},	// l
		//
		// Autodetect sequences for for VT100, State 5.
		//
		{0, ECHAR_CAST('\x70'),	ECHAR_CAST('\x70'),	emuAutoNothingVT100},	// p
		//
		// Autodetect sequences for for VT52, State 5.
		//
		{0, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	EmuStdChkZmdm}, 		// All
		//
		// State 6
		//
		{NEW_STATE, 0, 0, 0},
		//
		// Autodetect sequences for for VT52, VT100, State 6.
		//
		{6, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	nothing},				// All
		//
		// State 7
		//
		// Autodetect sequences for for VT100, State 7.
		//
		{NEW_STATE, 0, 0, 0},
		{7, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	EmuStdChkZmdm}, 		// All
		//
		// State 8
		//
		// Autodetect sequences for for VT100, State 8.
		//
		{NEW_STATE, 0, 0, 0},
		{8, ECHAR_CAST('\x00'),	ECHAR_CAST('\xFF'),	nothing},				// All
		};

	emuInstallStateTable(hhEmu, astfAutoAnsiTable, DIM(astfAutoAnsiTable));

	// Allocate space for and initialize data that is used only by the
	// Auto ANSI emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(ANSIPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTANSIPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(ANSIPRIVATE));

	hhEmu->emuResetTerminal = emuAnsiReset;

	// The Auto Detect emulator is ANSI based.	So, do the same
	// stuff we do when initializing the ANSI emulator.
	//
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
	hhEmu->emu_highchar = (ECHAR)0xFF;
#else
	hhEmu->emu_highchar = (ECHAR)0xFFFF;
#endif

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

// Ansi Auto Detect Functions.
//
void emuAutoNothingAnsi(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	nothing(hhEmu);
	return;
	}

void emuAutoScrollAnsi(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	vt_scrollrgn(hhEmu);
	return;
	}

void emuAutoSaveCurAnsi(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	ansi_savecursor(hhEmu);
	return;
	}

void emuAutoPnAnsi(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	ANSI_Pn(hhEmu);
	return;
	}

void emuAutoDoorwayAnsi(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	DoorwayMode(hhEmu);
	return;
	}

// VT52 Auto Detect Functions.
//
void emuAutoNothingVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	return;
	}

void emuAutoVT52toAnsi(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	vt52_toANSI(hhEmu);
	return;
	}

void emuAutoAnsiEdVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	ANSI_ED(hhEmu);
	return;
	}

void emuAutoAnsiElVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	ANSI_EL(hhEmu);
	return;
	}

void emuAutoCharPnVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	char_pn(hhEmu);
	return;
	}

// VT100 Auto Detect Functions.
//
void emuAutoNothingVT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	return;
	}

void emuAutoScs1VT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt_scs1(hhEmu);
	return;
	}

void emuAutoSaveCursorVT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt100_savecursor(hhEmu);
	return;
	}

void emuAutoAnsiPnEndVT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	ANSI_Pn_End(hhEmu);
	return;
	}

void emuAutoResetVT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt100_hostreset(hhEmu);
	return;
	}

void emuAutoAnsiDaVT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	ANSI_DA(hhEmu);
	return;
	}

void emuAutoReportVT100(const HHEMU hhEmu)
	{
#if 1
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt100_report(hhEmu);
	return;
	}

#if defined(INCL_VIEWDATA)
// Viewdata Auto Detect Functions.
//
void emuAutoNothingViewdata(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VIEW);
	return;
	}

void emuAutoSetAttrViewdata(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VIEW);
	EmuViewdataSetAttr(hhEmu);
	return;
	}
#endif // INCL_VIEWDATA

#if defined(INCL_MINITEL)

void emuAutoNothingMinitel(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	return;
	}

void emuAutoMinitelCharAttr(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	emuMinitelCharAttr(hhEmu);
	return;
	}

void emuAutoMinitelFieldAttr(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	emuMinitelFieldAttr(hhEmu);
	return;
	}

void emuAutoMinitelCursorReport(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	minitelCursorReport(hhEmu);
	return;
	}

#endif
