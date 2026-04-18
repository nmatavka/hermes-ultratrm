/*	File: \wacker\emu\vt220ini.c (Created: 24-Jan-1998)
 *
 *	Copyright 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
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

#if defined(INCL_VT220)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_init
 *
 * DESCRIPTION:
 *	 Initializes the VT220 emulator.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void vt220_init(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI;
	int iRow;

	static struct trans_entry const vt220_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
#if 1	// Left in from the VT100.
		{0, ECHAR_CAST('\x20'),	ECHAR_CAST('\x7E'),	emuDecGraphic}, 	// Space - ~
		{0, ECHAR_CAST('\xA0'),	ECHAR_CAST('\xFF'),	emuDecGraphic}, 	// 
#else
		{0, ECHAR_CAST('\x20'),	ECHAR_CAST('\x7E'),	emuDecGraphic}, 	// Space - ~
		{0, ECHAR_CAST('\xA0'),	0xFFFF,			emuDecGraphic}, 	// 
#endif

		{1, ECHAR_CAST('\x1B'),	ECHAR_CAST('\x1B'),	nothing},			// Esc
		{2, ECHAR_CAST('\x9B'),	ECHAR_CAST('\x9B'),	nothing},			// CSI

		// 7 bit control codes
//		{13,L'\x01',   ECHAR_CAST('\x01'),   nothing},			// Ctrl-A
		{0, ECHAR_CAST('\x05'),	ECHAR_CAST('\x05'),	vt100_answerback},	// Ctrl-E
		{0, ECHAR_CAST('\x07'),	ECHAR_CAST('\x07'),	emu_bell},			// Ctrl-G
		{0, ECHAR_CAST('\x08'),	ECHAR_CAST('\x08'),	vt_backspace},		// BackSpace
		{0, ECHAR_CAST('\x09'),	ECHAR_CAST('\x09'),	emuDecTab}, 		// Tab
		{0, ECHAR_CAST('\x0A'),	ECHAR_CAST('\x0C'),	emuLineFeed},		// NL - FF
		{0, ECHAR_CAST('\x0D'),	ECHAR_CAST('\x0D'),	carriagereturn},	// CR
		{0, ECHAR_CAST('\x0E'),	ECHAR_CAST('\x0F'),	vt_charshift},		// Ctrl-N, Ctrl-O
		{12,ECHAR_CAST('\x18'),	ECHAR_CAST('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X

		// 8 bit control codes
		{0, ECHAR_CAST('\x84'),	ECHAR_CAST('\x84'),	emuDecIND}, 		// Index cursor
		{0, ECHAR_CAST('\x85'),	ECHAR_CAST('\x85'),	ANSI_NEL}, 			// Next line
		{0, ECHAR_CAST('\x88'),	ECHAR_CAST('\x88'),	ANSI_HTS}, 			// Set Horizontal Tab
		{0, ECHAR_CAST('\x8D'),	ECHAR_CAST('\x8D'),	emuDecRI}, 			// Reverse index
		{0, ECHAR_CAST('\x8E'),	ECHAR_CAST('\x8F'),	vt_charshift}, 		// SingleShift G2,G3
		{5, ECHAR_CAST('\x90'),	ECHAR_CAST('\x90'),	nothing}, 			// Device Control String (DCS)

		// Ignore these codes. They just show what functionality is still missing.
		{0, ECHAR_CAST('\x00'),	ECHAR_CAST('\x00'),	nothing},			// ignore nuls
		{0, ECHAR_CAST('\x1A'),	ECHAR_CAST('\x1A'),	nothing},			// ignore Substitute
		{0, ECHAR_CAST('\x7F'),	ECHAR_CAST('\x7F'),	nothing},			// ignore Delete
		{0, ECHAR_CAST('\x9C'),	ECHAR_CAST('\x9C'),	nothing},			// ignore String Terminator


		{NEW_STATE, 0, 0, 0},   // State 1						// Esc
		{2, ECHAR_CAST('\x5B'),  ECHAR_CAST('\x5B'),  ANSI_Pn_Clr},		// '['
		{7, ECHAR_CAST('\x20'),  ECHAR_CAST('\x20'),  nothing},			// Space
		{3, ECHAR_CAST('\x23'),  ECHAR_CAST('\x23'),  nothing},			// #
		{4, ECHAR_CAST('\x28'),  ECHAR_CAST('\x2B'),  vt_scs1},			// ( - +
		{0, ECHAR_CAST('\x37'),  ECHAR_CAST('\x38'),  vt100_savecursor},  // 8
		{1, ECHAR_CAST('\x3B'),  ECHAR_CAST('\x3B'),  ANSI_Pn_End},		// ;
		{0, ECHAR_CAST('\x3D'),  ECHAR_CAST('\x3E'),  vt_alt_kpmode},		// = - >
		{0, ECHAR_CAST('\x44'),  ECHAR_CAST('\x44'),  emuDecIND},			// D
		{0, ECHAR_CAST('\x45'),  ECHAR_CAST('\x45'),  ANSI_NEL},			// E
		{0, ECHAR_CAST('\x48'),  ECHAR_CAST('\x48'),  ANSI_HTS},			// H
		{0, ECHAR_CAST('\x4D'),  ECHAR_CAST('\x4D'),  emuDecRI},			// M
		{0, ECHAR_CAST('\x4E'),  ECHAR_CAST('\x4F'),  vt_charshift},		// N - O
		{5, ECHAR_CAST('\x50'),  ECHAR_CAST('\x50'),  nothing},			// P
		{0, ECHAR_CAST('\x5A'),  ECHAR_CAST('\x5A'),  vt220_DA},			// Z
		{0, ECHAR_CAST('\\'),	ECHAR_CAST('\\'),	nothing},			// Backslash
		{0, ECHAR_CAST('\x63'),  ECHAR_CAST('\x63'),  vt220_hostreset},   // c
		{0, ECHAR_CAST('\x6E'),  ECHAR_CAST('\x6F'),  vt_charshift},		// n - o
		{0, ECHAR_CAST('\x7D'),  ECHAR_CAST('\x7E'),  vt_charshift},		// } - ~

		{NEW_STATE, 0, 0, 0},   // State 2						// ESC [
		{8, ECHAR_CAST('\x21'),  ECHAR_CAST('\x21'),  nothing},			// !
		{2, ECHAR_CAST('\x3B'),  ECHAR_CAST('\x3B'),  ANSI_Pn_End},		// ;
		{9, ECHAR_CAST('\x3E'),  ECHAR_CAST('\x3E'),  nothing},			// >
		{2, ECHAR_CAST('\x30'),  ECHAR_CAST('\x3F'),  ANSI_Pn},			// 0 - ?
		{11,ECHAR_CAST('\x22'),  ECHAR_CAST('\x22'),  nothing},			// "
//		{16,ECHAR_CAST('\x24'),  ECHAR_CAST('\x24'),  nothing},			// $
		{2, ECHAR_CAST('\x27'),  ECHAR_CAST('\x27'),  nothing},			// Eat Esc [ m ; m ; ' z
		{0, ECHAR_CAST('\x40'),  ECHAR_CAST('\x40'),  ANSI_ICH},			// @
		{0, ECHAR_CAST('\x41'),  ECHAR_CAST('\x41'),  emuDecCUU},			// A
		{0, ECHAR_CAST('\x42'),  ECHAR_CAST('\x42'),  emuDecCUD},			// B
		{0, ECHAR_CAST('\x43'),  ECHAR_CAST('\x43'),  emuDecCUF},			// C
		{0, ECHAR_CAST('\x44'),  ECHAR_CAST('\x44'),  emuDecCUB},			// D
		{0, ECHAR_CAST('\x48'),  ECHAR_CAST('\x48'),  emuDecCUP},			// H
		{0, ECHAR_CAST('\x4A'),  ECHAR_CAST('\x4A'),  emuVT220ED},		// J
		{0, ECHAR_CAST('\x4B'),  ECHAR_CAST('\x4B'),  emuDecEL},			// K
		{0, ECHAR_CAST('\x4C'),  ECHAR_CAST('\x4C'),  vt_IL},				// L
		{0, ECHAR_CAST('\x4D'),  ECHAR_CAST('\x4D'),  vt_DL},				// M
		{0, ECHAR_CAST('\x50'),  ECHAR_CAST('\x50'),  vt_DCH},			// P
		{0, ECHAR_CAST('\x58'),  ECHAR_CAST('\x58'),  vt_DCH},			// X
		{0, ECHAR_CAST('\x63'),  ECHAR_CAST('\x63'),  vt220_DA},			// c
		{0, ECHAR_CAST('\x66'),  ECHAR_CAST('\x66'),  emuDecCUP},			// f
		{0, ECHAR_CAST('\x67'),  ECHAR_CAST('\x67'),  ANSI_TBC},			// g
		{0, ECHAR_CAST('\x68'),  ECHAR_CAST('\x68'),  ANSI_SM},			// h
		{0, ECHAR_CAST('\x69'),  ECHAR_CAST('\x69'),  vt100PrintCommands},// i
		{0, ECHAR_CAST('\x6C'),  ECHAR_CAST('\x6C'),  ANSI_RM},			// l
		{0, ECHAR_CAST('\x6D'),  ECHAR_CAST('\x6D'),  ANSI_SGR},			// m
		{0, ECHAR_CAST('\x6E'),  ECHAR_CAST('\x6E'),  ANSI_DSR},			// n
		{0, ECHAR_CAST('\x71'),  ECHAR_CAST('\x71'),  nothing},			// q
		{0, ECHAR_CAST('\x72'),  ECHAR_CAST('\x72'),  vt_scrollrgn},		// r
		{0, ECHAR_CAST('\x75'),  ECHAR_CAST('\x75'),  nothing},			// u
		{0, ECHAR_CAST('\x79'),  ECHAR_CAST('\x79'),  nothing},			// y
		{0, ECHAR_CAST('\x7A'),  ECHAR_CAST('\x7A'),  nothing},			// z

		{NEW_STATE, 0, 0, 0},   // State 3						// Esc #
		{0, ECHAR_CAST('\x33'),  ECHAR_CAST('\x36'),  emuSetDoubleAttr},  // 3 - 6
		{0, ECHAR_CAST('\x38'),  ECHAR_CAST('\x38'),  vt_screen_adjust},  // 8

		{NEW_STATE, 0, 0, 0},   // State 4						// Esc ( - +
		{0, ECHAR_CAST('\x01'),  ECHAR_CAST('\xFF'),  vt_scs2},			// All

		{NEW_STATE, 0, 0, 0},   // State 5						// Esc P
		{5, ECHAR_CAST('\x3B'),  ECHAR_CAST('\x3B'),  ANSI_Pn_End},		// ;
		{5, ECHAR_CAST('\x30'),  ECHAR_CAST('\x3F'),  ANSI_Pn},			// 0 - ?
		{10,ECHAR_CAST('\x7C'),  ECHAR_CAST('\x7C'),  emuDecClearUDK},	// |

		{NEW_STATE, 0, 0, 0},   // State 6
		{6, ECHAR_CAST('\x00'),  ECHAR_CAST('\xFF'),  vt100_prnc},		// All

		{NEW_STATE, 0, 0, 0},   // State 7						// Esc Sapce
		{0, ECHAR_CAST('\x46'),  ECHAR_CAST('\x47'),  nothing},			// F - G

		{NEW_STATE, 0, 0, 0},   // State 8						// Esc [ !
		{0, ECHAR_CAST('\x70'),  ECHAR_CAST('\x70'),  vt220_softreset},   // p

		{NEW_STATE, 0, 0, 0},   // State 9						// Esc [ >
		{0, ECHAR_CAST('\x63'),  ECHAR_CAST('\x63'),  vt220_2ndDA},		// c

		{NEW_STATE, 0, 0, 0},   // State 10						// Esc P n;n |
		{10,ECHAR_CAST('\x00'),  ECHAR_CAST('\xFF'),  emuDecDefineUDK},   // All

		{NEW_STATE, 0, 0, 0},   // State 11						// Esc [ "
		{0, ECHAR_CAST('\x70'),  ECHAR_CAST('\x70'),  vt220_level},		// p
		{0, ECHAR_CAST('\x71'),  ECHAR_CAST('\x71'),  vt220_protmode},	// q

		{NEW_STATE, 0, 0, 0},   // State 12						// Ctrl-X
		{12,ECHAR_CAST('\x00'),  ECHAR_CAST('\xFF'),  EmuStdChkZmdm},		// All

		// States 13-17 are not used in HT but included for reference. 
//		{NEW_STATE, 0, 0, 0},   // State 13						// Ctrl-A
//		{14,ECHAR_CAST('\x08'),  ECHAR_CAST('\x08'),  emuSerialNbr},		// Backspace
//		{15,ECHAR_CAST('\x48'),  ECHAR_CAST('\x48'),  EmuStdChkHprP},		// H

//		{NEW_STATE, 0, 0, 0},   // State 14						// Ctrl-A bs
//		{14,ECHAR_CAST('\x00'),  ECHAR_CAST('\xFF'),  emuSerialNbr},		// All

//		{NEW_STATE, 0, 0, 0},   // State 15						// Ctrl-A H
//		{15,ECHAR_CAST('\x00'),  ECHAR_CAST('\xFF'),  EmuStdChkHprP},		// All

		// A real VT220/320 does not support the status line sequences.
//		{NEW_STATE, 0, 0, 0},   // State 16								// Esc [ n $
//		{16,ECHAR_CAST('\x7E'),  ECHAR_CAST('\x7E'),  emuDecSelectStatusLine},	// ~
//		{17,ECHAR_CAST('\x7D'),  ECHAR_CAST('\x7D'),  emuDecSelectActiveDisplay}, // }

//		{NEW_STATE, 0, 0, 0},   // State 17
//		{17,ECHAR_CAST('\x00'),  ECHAR_CAST('\xFF'),  emuDecStatusLineToss},  // All

		};

	// The following key tables were copied from \shared\emulator\vt220ini.c  
	// because they support user-defined keys. The tables have been modified 
	// so keydef.h is not needed and to match HT's use of keys. rde 2 Feb 98

	// The following key tables are defined in the order that they
	// are searched.
	//

	// These are the (standard) F1 thru F4 keys on the top and left of the
	// keyboard.  Note that these keys may be mapped to the top row of the
	// numeric keypad.  In that case, these keys (at the standard locations),
	// are not mapped to emulator keys. NOTE: HTPE does not use this mapping.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static  STEMUKEYDATA const VT220StdPfKeyTable[] =
		{
		EMUKEY(VK_F1,		1, 0, 0, 0, 0,  "\x8F\x50",			2), // P
		EMUKEY(VK_F2,		1, 0, 0, 0, 0,  "\x8F\x51",			2), // Q
		EMUKEY(VK_F3,		1, 0, 0, 0, 0,  "\x8F\x52",			2), // R
		EMUKEY(VK_F4,		1, 0, 0, 0, 0,  "\x8F\x53",			2), // S

		EMUKEY(VK_F1,		1, 0, 0, 0, 1,  "\x8F\x50",			2), // P
		EMUKEY(VK_F2,		1, 0, 0, 0, 1,  "\x8F\x51",			2), // Q
		EMUKEY(VK_F3,		1, 0, 0, 0, 1,  "\x8F\x52",			2), // R
		EMUKEY(VK_F4,		1, 0, 0, 0, 1,  "\x8F\x53",			2), // S
		};

	// When the user has selected the option to map the top 4 keys of the
	// numeric keypad to be the same as F1 thru F4, these key sequences are
	// used. NOTE: This is the mapping HTPE uses.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static  STEMUKEYDATA const VT220MovedPfKeyTable[] =
		{
		EMUKEY(VK_NUMLOCK,	1, 0, 0, 0, 1,  "\x8F\x50",			2), // P
		EMUKEY(VK_DIVIDE,	1, 0, 0, 0, 1,  "\x8F\x51",			2), // Q
		EMUKEY(VK_MULTIPLY,	1, 0, 0, 0, 1,  "\x8F\x52",			2), // R
		EMUKEY(VK_SUBTRACT,	1, 0, 0, 0, 1,  "\x8F\x53",			2), // S
		};

	// VT220 Keypad Numeric Mode.
	//
	static STEMUKEYDATA const VT220KeypadNumericMode[] =
		{
		// Keypad keys with Numlock off.
		//
		EMUKEY(VK_INSERT,	1, 0, 0, 0, 0,  "\x30",			1), // 0
		EMUKEY(VK_END,		1, 0, 0, 0, 0,  "\x31",			1), // 1
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 0,  "\x32",			1), // 2
		EMUKEY(VK_NEXT,		1, 0, 0, 0, 0,  "\x33",			1), // 3
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 0,  "\x34",			1), // 4
		EMUKEY(VK_NUMPAD5,	1, 0, 0, 0, 0,  "\x35",			1), // 5
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 0,  "\x36",			1), // 6
		EMUKEY(VK_HOME,		1, 0, 0, 0, 0,  "\x37",			1), // 7
		EMUKEY(VK_UP,		1, 0, 0, 0, 0,  "\x38",			1), // 8
		EMUKEY(VK_PRIOR,	1, 0, 0, 0, 0,  "\x39",			1), // 9
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 0,  "\x2E",			1), // .

		// Keypad keys with Numlock on.
		//
		EMUKEY(VK_NUMPAD0,		1, 0, 0, 0, 0,  "\x30",			1), // 0
		EMUKEY(VK_NUMPAD1,		1, 0, 0, 0, 0,  "\x31",			1), // 1
		EMUKEY(VK_NUMPAD2,		1, 0, 0, 0, 0,  "\x32",			1), // 2
		EMUKEY(VK_NUMPAD3,		1, 0, 0, 0, 0,  "\x33",			1), // 3
		EMUKEY(VK_NUMPAD4,		1, 0, 0, 0, 0,  "\x34",			1), // 4
		EMUKEY(VK_NUMPAD5,		1, 0, 0, 0, 0,  "\x35",			1), // 5
		EMUKEY(VK_NUMPAD6,		1, 0, 0, 0, 0,  "\x36",			1), // 6
		EMUKEY(VK_NUMPAD7,		1, 0, 0, 0, 0,  "\x37",			1), // 7
		EMUKEY(VK_NUMPAD8,		1, 0, 0, 0, 0,  "\x38",			1), // 8
		EMUKEY(VK_NUMPAD9,		1, 0, 0, 0, 0,  "\x39",			1), // 9
		EMUKEY(VK_DECIMAL,		1, 0, 0, 0, 0,  "\x2E",			1), // .

		// Other keypad keys (minus, plus, Enter).
		//
		EMUKEY(VK_SUBTRACT,		1, 0, 0, 0, 0,  "\x2D",			1), // -
		EMUKEY(VK_ADD,			1, 0, 0, 0, 0,  "\x2C",			1), // ,
		EMUKEY(VK_RETURN,		1, 0, 0, 0, 1,  "\x0D",			1), // CR
		};

	// VT220 Keypad Application Mode.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static STEMUKEYDATA const VT220KeypadApplicationMode[] =
		{
		// Keypad keys with Numlock off.
		//
		EMUKEY(VK_NUMPAD0,		1, 0, 0, 0, 0,  "\x8F\x70",		2), // p
		EMUKEY(VK_NUMPAD1,		1, 0, 0, 0, 0,  "\x8F\x71",		2), // q
		EMUKEY(VK_NUMPAD2,		1, 0, 0, 0, 0,  "\x8F\x72",		2), // r
		EMUKEY(VK_NUMPAD3,		1, 0, 0, 0, 0,  "\x8F\x73",		2), // s
		EMUKEY(VK_NUMPAD4,		1, 0, 0, 0, 0,  "\x8F\x74",		2), // t
		EMUKEY(VK_NUMPAD5,		1, 0, 0, 0, 0,  "\x8F\x75",		2), // u
		EMUKEY(VK_NUMPAD6,		1, 0, 0, 0, 0,  "\x8F\x76",		2), // v
		EMUKEY(VK_NUMPAD7,		1, 0, 0, 0, 0,  "\x8F\x77",		2), // w
		EMUKEY(VK_NUMPAD8,		1, 0, 0, 0, 0,  "\x8F\x78",		2), // x
		EMUKEY(VK_NUMPAD9,		1, 0, 0, 0, 0,  "\x8F\x79",		2), // y
		EMUKEY(VK_DECIMAL,		1, 0, 0, 0, 0,  "\x8F\x6E",		2), // n

		// Keypad keys with Numlock on.
		//
		EMUKEY(VK_NUMPAD0,		1, 0, 0, 0, 0,  "\x8F\x70",		2), // p
		EMUKEY(VK_NUMPAD1,		1, 0, 0, 0, 0,  "\x8F\x71",		2), // q
		EMUKEY(VK_NUMPAD2,		1, 0, 0, 0, 0,  "\x8F\x72",		2), // r
		EMUKEY(VK_NUMPAD3,		1, 0, 0, 0, 0,  "\x8F\x73",		2), // s
		EMUKEY(VK_NUMPAD4,		1, 0, 0, 0, 0,  "\x8F\x74",		2), // t
		EMUKEY(VK_NUMPAD5,		1, 0, 0, 0, 0,  "\x8F\x75",		2), // u
		EMUKEY(VK_NUMPAD6,		1, 0, 0, 0, 0,  "\x8F\x76",		2), // v
		EMUKEY(VK_NUMPAD7,		1, 0, 0, 0, 0,  "\x8F\x77",		2), // w
		EMUKEY(VK_NUMPAD8,		1, 0, 0, 0, 0,  "\x8F\x78",		2), // x
		EMUKEY(VK_NUMPAD9,		1, 0, 0, 0, 0,  "\x8F\x79",		2), // y
		EMUKEY(VK_DECIMAL,		1, 0, 0, 0, 0,  "\x8F\x6E",		2), // n

		// Other keypad keys (minus, plus, Enter).
		//
		EMUKEY(VK_SUBTRACT,		1, 0, 0, 0, 0,  "\x8F\x6D",		2), // m
		EMUKEY(VK_ADD,			1, 0, 0, 0, 0,  "\x8F\x6C",		2), // l
		EMUKEY(VK_RETURN,		1, 0, 0, 0, 1,  "\x8F\x4D",		2), // M
		};

	// VT220 Cursor Key Mode.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static STEMUKEYDATA const VT220CursorKeyMode[] =
		{
		// Arrow keys on the numeric keypad.  These sequences are used
		// when the emulator is using Cursor Key Mode (Application Keys).
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 0,  "\x8F\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 0,  "\x8F\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 0,  "\x8F\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 0,  "\x8F\x44",			2), // D

		// Arrow keys on the edit pad.  These sequences are used
		// when the emulator is using Cursor Key Mode (Application Keys).
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 1,  "\x8F\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 1,  "\x8F\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 1,  "\x8F\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 1,  "\x8F\x44",			2), // D
		};

	// VT220 Standard Key Table.
	//
	static STEMUKEYDATA const VT220StandardKeys[] =
		{
		// Some keys on the numeric keypad will respond in the same
		// way the corresponding keys on the edit pad respond.
		//
		EMUKEY(VK_HOME,		1, 0, 0, 0, 0,  "\x9B\x31\x7E",		3), // 1 ~
		EMUKEY(VK_INSERT,	1, 0, 0, 0, 0,  "\x9B\x32\x7E",		3), // 2 ~
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 0,  "\x9B\x33\x7E",		3), // 3 ~
		EMUKEY(VK_END,		1, 0, 0, 0, 0,  "\x9B\x34\x7E",		3), // 4 ~
		EMUKEY(VK_PRIOR,	1, 0, 0, 0, 0,  "\x9B\x35\x7E",		3), // 5 ~
		EMUKEY(VK_NEXT,		1, 0, 0, 0, 0,  "\x9B\x36\x7E",		3), // 6 ~

		// These are the keys on the edit pad.
		//
		EMUKEY(VK_HOME,		1, 0, 0, 0, 1,  "\x9B\x31\x7E",		3), // 1 ~
		EMUKEY(VK_INSERT,	1, 0, 0, 0, 1,  "\x9B\x32\x7E",		3), // 2 ~
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 1,  "\x9B\x33\x7E",		3), // 3 ~
		EMUKEY(VK_END,		1, 0, 0, 0, 1,  "\x9B\x34\x7E",		3), // 4 ~
		EMUKEY(VK_PRIOR,	1, 0, 0, 0, 1,  "\x9B\x35\x7E",		3), // 5 ~
		EMUKEY(VK_NEXT,		1, 0, 0, 0, 1,  "\x9B\x36\x7E",		3), // 6 ~

		// Arrow keys on the numeric keypad.
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 0,  "\x9B\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 0,  "\x9B\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 0,  "\x9B\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 0,  "\x9B\x44",			2), // D

		// Arrow keys on the edit pad.
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 1,  "\x9B\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 1,  "\x9B\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 1,  "\x9B\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 1,  "\x9B\x44",			2), // D

		// Function keys (F5)F6 thru F10.
		//
#if defined(INCL_ULTC_VERSION)
		EMUKEY(VK_F5,		1, 0, 0, 0, 0,  "\x9B\x31\x36\x7E", 4), // 1 6 ~
#endif
		EMUKEY(VK_F6,		1, 0, 0, 0, 0,  "\x9B\x31\x37\x7E", 4), // 1 7 ~
		EMUKEY(VK_F7,		1, 0, 0, 0, 0,  "\x9B\x31\x38\x7E", 4), // 1 8 ~
		EMUKEY(VK_F8,		1, 0, 0, 0, 0,  "\x9B\x31\x39\x7E", 4), // 1 9 ~
		EMUKEY(VK_F9,		1, 0, 0, 0, 0,  "\x9B\x32\x30\x7E", 4), // 2 0 ~
		EMUKEY(VK_F10,		1, 0, 0, 0, 0,  "\x9B\x32\x31\x7E", 4), // 2 1 ~

#if defined(INCL_ULTC_VERSION)
		EMUKEY(VK_F5,		1, 0, 0, 0, 1,  "\x9B\x31\x36\x7E", 4), // 1 6 ~
#endif
		EMUKEY(VK_F6,		1, 0, 0, 0, 1,  "\x9B\x31\x37\x7E", 4), // 1 7 ~
		EMUKEY(VK_F7,		1, 0, 0, 0, 1,  "\x9B\x31\x38\x7E", 4), // 1 8 ~
		EMUKEY(VK_F8,		1, 0, 0, 0, 1,  "\x9B\x31\x39\x7E", 4), // 1 9 ~
		EMUKEY(VK_F9,		1, 0, 0, 0, 1,  "\x9B\x32\x30\x7E", 4), // 2 0 ~
		EMUKEY(VK_F10,		1, 0, 0, 0, 1,  "\x9B\x32\x31\x7E", 4), // 2 1 ~

		// Function keys F11 thru F20 are invoked by the user pressing
		// Ctrl-F1 thru Ctrl-F10.
		//
		// Function keys Ctrl-F1 thru Ctrl-F10 (Top row).
		//
		EMUKEY(VK_F1,		1, 1, 0, 0, 0,  "\x9B\x32\x33\x7E", 4), // 2 3 ~
		EMUKEY(VK_F2,		1, 1, 0, 0, 0,  "\x9B\x32\x34\x7E", 4), // 2 4 ~
		EMUKEY(VK_F3,		1, 1, 0, 0, 0,  "\x9B\x32\x35\x7E", 4), // 2 5 ~
		EMUKEY(VK_F4,		1, 1, 0, 0, 0,  "\x9B\x32\x36\x7E", 4), // 2 6 ~
		EMUKEY(VK_F5,		1, 1, 0, 0, 0,  "\x9B\x32\x38\x7E", 4), // 2 8 ~
		EMUKEY(VK_F6,		1, 1, 0, 0, 0,  "\x9B\x32\x39\x7E", 4), // 2 9 ~
		EMUKEY(VK_F7,		1, 1, 0, 0, 0,  "\x9B\x33\x31\x7E", 4), // 3 1 ~
		EMUKEY(VK_F8,		1, 1, 0, 0, 0,  "\x9B\x33\x32\x7E", 4), // 3 2 ~
		EMUKEY(VK_F9,		1, 1, 0, 0, 0,  "\x9B\x33\x33\x7E", 4), // 3 3 ~
		EMUKEY(VK_F10,		1, 1, 0, 0, 0,  "\x9B\x33\x34\x7E", 4), // 3 4 ~

		EMUKEY(VK_F1,		1, 1, 0, 0, 1,  "\x9B\x32\x33\x7E", 4), // 2 3 ~
		EMUKEY(VK_F2,		1, 1, 0, 0, 1,  "\x9B\x32\x34\x7E", 4), // 2 4 ~
		EMUKEY(VK_F3,		1, 1, 0, 0, 1,  "\x9B\x32\x35\x7E", 4), // 2 5 ~
		EMUKEY(VK_F4,		1, 1, 0, 0, 1,  "\x9B\x32\x36\x7E", 4), // 2 6 ~
		EMUKEY(VK_F5,		1, 1, 0, 0, 1,  "\x9B\x32\x38\x7E", 4), // 2 8 ~
		EMUKEY(VK_F6,		1, 1, 0, 0, 1,  "\x9B\x32\x39\x7E", 4), // 2 9 ~
		EMUKEY(VK_F7,		1, 1, 0, 0, 1,  "\x9B\x33\x31\x7E", 4), // 3 1 ~
		EMUKEY(VK_F8,		1, 1, 0, 0, 1,  "\x9B\x33\x32\x7E", 4), // 3 2 ~
		EMUKEY(VK_F9,		1, 1, 0, 0, 1,  "\x9B\x33\x33\x7E", 4), // 3 3 ~
		EMUKEY(VK_F10,		1, 1, 0, 0, 1,  "\x9B\x33\x34\x7E", 4), // 3 4 ~

		EMUKEY(VK_F1,		1, 0, 0, 0, 0,	"\x8FP",			2),
		EMUKEY(VK_F2,		1, 0, 0, 0, 0,	"\x8FQ",			2),
		EMUKEY(VK_F3,		1, 0, 0, 0, 0,	"\x8FR",			2),
		EMUKEY(VK_F4,		1, 0, 0, 0, 0,	"\x8FS",			2),

		EMUKEY(VK_F1,		1, 0, 0, 0, 1,	"\x8FP",			2),
		EMUKEY(VK_F2,		1, 0, 0, 0, 1,	"\x8FQ",			2),
		EMUKEY(VK_F3,		1, 0, 0, 0, 1,	"\x8FR",			2),
		EMUKEY(VK_F4,		1, 0, 0, 0, 1,	"\x8FS",			2),

		EMUKEY(VK_DELETE,	1, 0, 0, 0, 0,	"\x7F",				1),	// KN_DEL
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 1,	"\x7F",				1),	// KN_DEL

		EMUKEY(VK_ADD,		1, 0, 0, 0, 0,	",",				1),

		// Ctrl-2.
		// Ctrl-@.
		//
		EMUKEY(0x32,		1, 1, 0, 0, 0,  "\x00",				1),
		EMUKEY(0x32,		1, 1, 0, 1, 0,  "\x00",				1),

		// Ctrl-6.
		// Ctrl-^.
		EMUKEY(0x36,		1, 1, 0, 0, 0,  "\x1E",				1),
		EMUKEY(0x36,		1, 1, 0, 1, 0,  "\x1E",				1),

		// Ctrl-Space
		//
		EMUKEY(VK_SPACE,	1, 1, 0, 0, 0,  "\x00",				1),

		// Ctrl-- key.
		//
		EMUKEY(VK_SUBTRACT,	1, 1, 0, 0, 1,  "\x1F",				1),
		};

	// VT220 User Defined keys.
	static STEMUKEYDATA VT220UserDefinedKeys[MAX_UDK_KEYS] =
		{
		// NOTE: Do not change the order of these user defined entries.
		// emuDecDefineUDK assumes a 1:1 correspondance with this
		// table and the UDKSelector table defined below.
		//
		// Initialize Virtual and Shift flags.
		//
		EMUKEY(VK_F6,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F7,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F8,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F9,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F10,		1, 0, 0, 1, 0,  0,					0),

		// Initialize Virtual and Alt flags.
		//
		EMUKEY(VK_F1,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F2,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F3,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F4,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F5,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F6,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F7,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F8,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F9,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F10,		1, 0, 1, 0, 0,  0,					0),
		};

	// NOTE: Do not change the order of these entries.
	// There is a 1:1 correspondance between this table and the
	// user defined key table defined above.
	//
	static WCHAR const acUDKSelectors[MAX_UDK_KEYS] =
		{
		L'\x17', L'\x18', L'\x19', L'\x20', // F6 -  F9
		L'\x21', L'\x23', L'\x24', L'\x25', // F10 - F13
		L'\x26', L'\x28', L'\x29', L'\x31', // F14 - F17
		L'\x32', L'\x33', L'\x34',				// F18 - F20
		};

	emuInstallStateTable(hhEmu, vt220_tbl, DIM(vt220_tbl));

	// Allocate space for and initialize data that is used only by the
	// VT220 emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(DECPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(DECPRIVATE));

	// NOTE:	The order of these definitions directly correspond to the
	//			search order used by the emuDecKeyboardIn function.
	//			Don't change these.
	//
	// In shared code, these are all part of hhEmu.
	pstPRI->pstcEmuKeyTbl1 = VT220StdPfKeyTable;
	pstPRI->pstcEmuKeyTbl2 = VT220MovedPfKeyTable;
	pstPRI->pstcEmuKeyTbl3 = VT220KeypadNumericMode;
	pstPRI->pstcEmuKeyTbl4 = VT220KeypadApplicationMode;
	pstPRI->pstcEmuKeyTbl5 = VT220CursorKeyMode;
	pstPRI->pstcEmuKeyTbl6 = VT220StandardKeys;

	pstPRI->iKeyTable1Entries = DIM(VT220StdPfKeyTable);
	pstPRI->iKeyTable2Entries = DIM(VT220MovedPfKeyTable);
	pstPRI->iKeyTable3Entries = DIM(VT220KeypadNumericMode);
	pstPRI->iKeyTable4Entries = DIM(VT220KeypadApplicationMode);
	pstPRI->iKeyTable5Entries = DIM(VT220CursorKeyMode);
	pstPRI->iKeyTable6Entries = DIM(VT220StandardKeys);

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

	// Initialize hhEmu values for VT220.
	//
	hhEmu->emu_setcurpos	= emuDecSetCurPos;
	hhEmu->emu_deinstall	= emuDecUnload;
	hhEmu->emu_clearline	= emuDecClearLine;
	hhEmu->emu_clearscreen  = emuDecClearScreen;
	hhEmu->emu_kbdin		= emuDecKeyboardIn;
	hhEmu->emuResetTerminal = vt220_reset;
	hhEmu->emu_graphic		= emuDecGraphic;
//	hhEmu->emu_scroll		= emuDecScroll;

#if 1
	hhEmu->emu_highchar 	= 0x7E;
#else
	hhEmu->emu_highchar 	= 0xFFFF;
#endif

	hhEmu->emu_maxcol		= VT_MAXCOL_80MODE;
	hhEmu->fUse8BitCodes		= FALSE;
	hhEmu->mode_vt220			= FALSE;
	hhEmu->mode_vt320			= FALSE;
	//hhEmu->vt220_protectmode	= FALSE;
	hhEmu->mode_protect			= FALSE;

	if (hhEmu->nEmuLoaded == EMU_VT220)
		hhEmu->mode_vt220 = TRUE;

	else if (hhEmu->nEmuLoaded == EMU_VT320)
		hhEmu->mode_vt320 = TRUE;

	else
		assert(FALSE);

// UNDO:rde
//	pstPRI->vt220_protimg = 0;

	pstPRI->pstUDK			= VT220UserDefinedKeys;
	pstPRI->iUDKTableEntries= DIM(VT220UserDefinedKeys);

	pstPRI->pacUDKSelectors = acUDKSelectors;
	pstPRI->iUDKState		= KEY_NUMBER_NEXT;

	std_dsptbl(hhEmu, TRUE);
	vt_charset_init(hhEmu);

	switch(hhEmu->stUserSettings.nEmuId)
		{
	case EMU_VT220:
		hhEmu->mode_vt220		= TRUE;
		hhEmu->mode_vt320		= FALSE;
		vt220_reset(hhEmu, FALSE);
		break;

	case EMU_VT320:
		hhEmu->mode_vt220		= FALSE;
		hhEmu->mode_vt320		= TRUE;
		break;

	default:
		assert(FALSE);
		break;
		}

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}
#endif // INCL_VT220

/* end of vt220ini.c */
