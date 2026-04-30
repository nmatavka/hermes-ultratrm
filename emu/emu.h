/*	File: D:\WACKER\emu\emu.h (Created: 08-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 5 $
 *	$Date: 3/16/01 4:28p $
 */

/* Emulator ID's */

// Don't ever change emulator id numbers.  They are stored in session files
// and as such are cast in stone. - mrw,4/13/95
//
#define EMU_AUTO						100
#define EMU_ANSI						101
#define EMU_MINI						102
#define EMU_VIEW						109
#define EMU_TTY 						110
#define EMU_VT100						111
#define EMU_VT220						112	// rde:23 Jan 98
#define EMU_VT320						113	// rde:23 Jan 98
#define EMU_VT52						115
#define EMU_VT100J						116
#define EMU_ANSIW						117
#define EMU_VT100PLUS					118 // REV: 02/28/2001 
#define EMU_VTUTF8                      119 // REV: 02/28/2001 
#define EMU_IBM3270                      120
#define EMU_IBM5250                      121
#define EMU_VIDEOTEX                    122

#define NBR_EMULATORS					16

/* Emulator constants  */

// Note: if you change MAX_EMUROWS or MAX_EMUCOLS also change
//		 TERM_ROWS and TERM_COLS in term.hh to match - mrw
// Note: Can't find TERM_ROWS and TERM_COLS. rde 10 Jun 98

#define MAX_EMUROWS 	50		// Largest vertical size for emulator.
#define MAX_EMUCOLS 	132		// Largest horizontal size for emulator.
#define MIN_EMUROWS 	10		// Smallest allowable value for rows,
#define MIN_EMUCOLS 	20		// and columns.

#define EMU_DEFAULT_COLS			80
#define EMU_DEFAULT_ROWS			24

#define EMU_OK						0
#define TRM_NUMSTRIPCHARS			3
#define EMU_MAX_NAMELEN 			15
#define EMU_MAX_AUTODETECT_ATTEMPTS 10
#define EMU_MAX_TELNETID            256
#define EMU_IBM3270_MAX_LUNAME       64
#define EMU_IBM3270_MAX_PATH         260
#define EMU_IBM3270_MAX_CODEPAGE     32
#define EMU_IBM3270_MAX_OPTIONS      260
#define EMU_IBM5250_MAX_NAME         64
#define EMU_IBM5250_MAX_PATH         260
#define EMU_IBM5250_MAX_CODEPAGE     32
#define EMU_IBM5250_MAX_OPTIONS      260
#define EMU_VIDEOTEX_MAX_PROFILE    32
#define EMU_VIDEOTEX_MAX_HOST       128

#define EMU_IBM3270_TLS_OFF          0
#define EMU_IBM3270_TLS_DIRECT       1
#define EMU_IBM3270_TLS_STARTTLS     2
#define EMU_IBM5250_TLS_OFF          0
#define EMU_IBM5250_TLS_DIRECT       1

#define EMU_COLOR_THEME_SOLARIZED_LIGHT  0
#define EMU_COLOR_THEME_SOLARIZED_DARK   1
#define EMU_COLOR_THEME_SELENIZED_LIGHT  2
#define EMU_COLOR_THEME_SELENIZED_DARK   3
#define EMU_COLOR_THEME_SELENIZED_BLACK  4
#define EMU_COLOR_THEME_SELENIZED_WHITE  5
#define EMU_COLOR_THEME_WINDOWS          6
#define EMU_COLOR_THEME_COUNT            7

// JCM should remove the two defines below.
//
#define EMU_DEFAULT_MAXCOL			79
#define EMU_DEFAULT_MAXROW			23

#define EMU_KEYS_ACCEL		0
#define EMU_KEYS_TERM		1
#define EMU_KEYS_SCAN		2

#define EMU_CURSOR_BLOCK	1
#define EMU_CURSOR_LINE 	2
#define EMU_CURSOR_NONE 	3

#define EMU_CHARSET_ASCII			0
#define EMU_CHARSET_UK				1
#define EMU_CHARSET_SPECIAL			2
#define EMU_CHARSET_MULTINATIONAL	3
#define EMU_CHARSET_FRENCH			4
#define EMU_CHARSET_FRENCHCANADIAN	5
#define EMU_CHARSET_GERMAN			6

#define EMU_EVENT_CONNECTED 	1
#define EMU_EVENT_DISCONNECTED	2
#define EMU_EVENT_CONNECTING	3

#define EMU_BKSPKEYS_CTRLH		1
#define EMU_BKSPKEYS_DEL		2
#define EMU_BKSPKEYS_CTRLHSPACE 3

// 8 bits just ain't enough anymore.  Going to bit fields to handle
// things like text marking, blinking, underlining, etc.  Also can
// handle more colors this way if we want.	For now though stick to
// original scheme of 4 bits for foreground color and 4 bits for
// background color.

struct stAttribute
	{
	unsigned int txtclr : 4;		// text or foreground color index.
	unsigned int bkclr	: 4;		// background color index.
	unsigned int txtmrk : 1;		// true if text is 'marked'.
	unsigned int undrln : 1;		// underline
	unsigned int hilite : 1;		// foreground intensity
	unsigned int bklite : 1;		// background intensity
	unsigned int blink	: 1;		// soon to be famous blink attribute
	unsigned int revvid : 1;		// reverse video
	unsigned int blank	: 1;		// blank attribute
	unsigned int dblwilf: 1;		// double wide left
	unsigned int dblwirt: 1;		// double wide right
	unsigned int dblhilo: 1;		// double height top half
	unsigned int dblhihi: 1;		// double height bottom half
    unsigned int protect: 1;        // protected bit for DEC emulators.
	unsigned int symbol:  1;		// use symbol font
	unsigned int wilf	: 1;		// wide left
	unsigned int wirt	: 1;		// wide right
	};

typedef struct stAttribute STATTR;
typedef STATTR *PSTATTR;

// Note: This structure is now used only internally to the program--it is
// no longer used to load in and save out data in the session file. 
// Consequently, it is safe to add and remove items at will. rde 8 Jun 98
struct emuSettings
	{
	int 	nEmuId, 			// 100 = EMU_AUTO
								// 101 = EMU_ANSI
								// 102 = EMU_MINI
								// 109 = EMU_VIEW
								// 110 = EMU_TTY
								// 111 = EMU_VT100
								// 112 = EMU_VT220
								// 113 = EMU_VT320
								// 115 = EMU_VT52
								// 116 = EMU_VT100J
                                // 117 = EMU_ANSIW
								// 118 = EMU_VT100PLUS
								// 119 = EMU_VTUTF8
								// 120 = EMU_IBM3270
								// 121 = EMU_IBM5250
								// 122 = EMU_VIDEOTEX
								//
			nTermKeys,			// 0 = EMU_KEYS_ACCEL
								// 1 = EMU_KEYS_TERM
								// 2 = EMU_KEYS_SCAN
								//
			nCursorType,		// 1 = EMU_CURSOR_BLOCK
								// 2 = EMU_CURSOR_LINE
	        					// 3 = EMU_CURSOR_NONE
								//
			nCharacterSet,		// 0 = EMU_CHARSET_ASCII
								// 1 = EMU_CHARSET_UK
								// 2 = EMU_CHARSET_SPECIAL
								//
			nAutoAttempts,		// Count of connections using the Auto
								// Detect Emulator.  At
								// EMU_MAX_AUTODETECT_ATTEMPTS, we switch
								// to Ansi emulation.  Note, this may
								// get moved into a Statictics Handle
								// if we ever develop one.
								//
			fCursorBlink,		// Blinking cursor. 			True\False.
			fMapPFkeys, 		// PF1-PF4 to top row of keypad.True\False.
			fAltKeypadMode, 	// Alternate keypad mode.		True\False.
			fKeypadAppMode, 	// Keypad application mode. 	True\False.
			fCursorKeypadMode,	// Cursor keypad mode.			True\Fales.
			fReverseDelBk,		// Reverse Del and Backsp.		True\False.
			f132Columns,		// 132 column display.			True\False.
			fDestructiveBk, 	// Destructive backspace.		True\False.
			fWrapLines, 		// Wrap lines.					True\False.
			fLbSymbolOnEnter,	// Send # symbol on Enter.		True\False.

	// Note: The following two variables were added for the VT220/320. rde:24 Jan 98
            fUse8BitCodes,      // 8-bit control codes          True\False.
            fAllowUserKeys,     // User defined keys allowed    True\False.
    // Note: The following variable was added for VT100/220/320. mpt:5-18-00
			fPrintRaw;  		// Do not use windows print drv True\False.
                                
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
	// The following four variables were added for user settable
	// terminal screen size and colors. rde 1 Jun 98
	int		nColorTheme,		// EMU_COLOR_THEME_* terminal palette.
			nTextColor,			// Default text color.			0 thru 15.
			nBackgroundColor,	// Default background color.	0 thru 15.
			nUserDefRows,		// Number of terminal rows.		12 thru 50.
			nUserDefCols;		// Number of terminal columns.	40 thru 132.
#endif

    // Note: The following two variables are only used if the "Include
    // User Defined Backspace and Telnet Terminal Id" feature is enabled.
    // There is no compile switch here because this entire structure gets
    // written to the session file in one large chunk. Using a compile
    // switch could potentially cause version problems later on down
    // the road. - cab:11/15/96
    //
    int     nBackspaceKeys;     // 1 = EMU_BKSPKEYS_CTRLH
                                // 2 = EMU_BKSPKEYS_DEL
                                // 3 = EMU_BKSPKEYS_CTRLHSPACE

    WCHAR   acTelnetId[EMU_MAX_TELNETID];   // Terminal type/device ID

    int     nIbm3270Model;       // 2, 3, 4, or 5.
    int     fIbm3270Tls;         // TLS requested for direct IBM 3270 connections.
    int     fIbm3270Printer;     // Associated 3287 printer requested.
    int     nIbm3270TlsMode;     // 0 off, 1 direct TLS, 2 STARTTLS.
    int     fIbm3270VerifyCert;  // Verify TLS certificates when TLS is used.
    WCHAR   acIbm3270LuName[EMU_IBM3270_MAX_LUNAME];
    WCHAR   acIbm3270DeviceName[EMU_IBM3270_MAX_LUNAME];
    WCHAR   acIbm3270HostCodePage[EMU_IBM3270_MAX_CODEPAGE];
    WCHAR   acIbm3270CaPath[EMU_IBM3270_MAX_PATH];
    WCHAR   acIbm3270CertPath[EMU_IBM3270_MAX_PATH];
    WCHAR   acIbm3270KeyPath[EMU_IBM3270_MAX_PATH];
    WCHAR   acIbm3270IndFileDir[EMU_IBM3270_MAX_PATH];
    WCHAR   acIbm3270IndFileOptions[EMU_IBM3270_MAX_OPTIONS];
    WCHAR   acIbm3270PrinterLu[EMU_IBM3270_MAX_LUNAME];
    WCHAR   acIbm3270PrinterOptions[EMU_IBM3270_MAX_OPTIONS];
    int     fIbm5250Tls;         // TLS requested for direct IBM 5250 connections.
    int     fIbm5250Printer;     // Associated 5250 printer requested.
    int     nIbm5250TlsMode;     // 0 off, 1 direct TLS.
    int     fIbm5250VerifyCert;  // Verify TLS certificates when TLS is used.
    WCHAR   acIbm5250TermType[EMU_IBM5250_MAX_NAME];
    WCHAR   acIbm5250DeviceName[EMU_IBM5250_MAX_NAME];
    WCHAR   acIbm5250HostCodePage[EMU_IBM5250_MAX_CODEPAGE];
    WCHAR   acIbm5250CaPath[EMU_IBM5250_MAX_PATH];
    WCHAR   acIbm5250CertPath[EMU_IBM5250_MAX_PATH];
    WCHAR   acIbm5250KeyPath[EMU_IBM5250_MAX_PATH];
    WCHAR   acIbm5250PrinterDevice[EMU_IBM5250_MAX_NAME];
    WCHAR   acIbm5250PrinterOptions[EMU_IBM5250_MAX_OPTIONS];
    WCHAR   acVideotexProfile[EMU_VIDEOTEX_MAX_PROFILE];
    WCHAR   acVideotexHost[EMU_VIDEOTEX_MAX_HOST];
    int     nVideotexPort;
    int     nVideotexFontMap;
    int     fVideotexTelnetCompat;
	};

typedef struct emuSettings STEMUSET;
typedef STEMUSET *PSTEMUSET;


/* emuhdl.c */
HEMU	emuCreateHdl(const HSESSION hSession);
int 	emuDestroyHdl(const HEMU hEmu);
int 	emuLoad(const HEMU hEmu, const int nEmuId);
void	emuLock(const HEMU hEmu);
void	emuUnlock(const HEMU hEmu);
ECHAR	**emuGetTxtBuf(const HEMU hEmu);
PSTATTR *emuGetAttrBuf(const HEMU hEmu);
int 	emuKbdIn(const HEMU hEmu, KEY_T key, const int fTest);
int 	emuDataIn(const HEMU hEmu, const ECHAR ccode);
int 	emuComDone(const HEMU hEmu);
int 	emuTrackingNotify(const HEMU hEmu);
int 	emuIsEmuKey(const HEMU hEmu, KEY_T key);
int 	emuQueryClearAttr(const HEMU hemu, PSTATTR pstClearAttr);
int 	emuQueryCurPos(const HEMU hEmu, int *row, int *col);
int     emuSetCurPos(const HEMU hEmu, int row, int col);
HPRINT	emuQueryPrintEchoHdl(const HEMU hEmu);
int 	emuQueryRowsCols(const HEMU hEmu, int *piRows, int *piCols);
int     emuCopyTextRange(const HEMU hEmu,
                         int iRow,
                         int iCol,
                         int cchCount,
                         WCHAR *pszBuffer,
                         int cchBuffer);
int 	emuQueryEmulatorId(const HEMU hEmulator);
int 	emuNotify(const HEMU hEmu, const int nEvent);
int 	emuQueryCursorType(const HEMU hEmu);
int 	emuQueryName(const HEMU hEmu, WCHAR *achBuffer, int nSize);
int 	emuSetSettings(const HEMU hEmu, const PSTEMUSET pstSettings);
int 	emuQuerySettings(const HEMU hEmu, PSTEMUSET pstSettings);
int 	emuInitializeHdl(const HEMU hEmu);
int 	emuSaveHdl(const HEMU hEmu);
int 	emuHomeHostCursor(const HEMU hEmu);
int 	emuEraseTerminalScreen(const HEMU hEmu);

void	emuMinitelSendKey(const HEMU hEmu, const int iCmd); // minitel.c
int 	emuGetIdFromName(const HEMU hEmu, WCHAR *achEmuName);

int     emuQueryDefaultTelnetId(const int nEmuId, WCHAR *achTelnetId, int nSize);
const WCHAR *emuQueryIbmProtocolName(const int nEmuId, const int nTransport);
int     emuLoadDefaultTelnetId(const HEMU hEmu);
int     emuIbm3270RecordIn(const HEMU hEmu, const unsigned char *buf, int len);
void    emuIbm3270SetTelnetMode(const HEMU hEmu, int fIbm3270E);
int     emuIbm3270StartPrinter(const HEMU hEmu);
int     emuIbm3270StopPrinter(const HEMU hEmu);
int     emuIbm3270IndFileSend(const HEMU hEmu);
int     emuIbm3270IndFileReceive(const HEMU hEmu);
int     emuIbm5250BytesIn(const HEMU hEmu, const unsigned char *buf, int len);
int     emuIbm5250StartPrinter(const HEMU hEmu);
int     emuIbm5250StopPrinter(const HEMU hEmu);
int     emuIbm5250ShowSettings(const HEMU hEmu);
int     emuVideotexBytesIn(const HEMU hEmu, const unsigned char *buf, int len);
int     emuVideotexShowSettings(const HEMU hEmu);

/*	colors indexes */

#define VC_BLACK		0
#define VC_BLUE 		1
#define VC_GREEN		2
#define VC_CYAN 		3
#define VC_RED			4
#define VC_MAGENTA		5
#define VC_BROWN		6
#define VC_WHITE		7
#define VC_GRAY 		8
#define VC_BRT_BLUE 	9
#define VC_BRT_GREEN	10
#define VC_BRT_CYAN 	11
#define VC_BRT_RED		12
#define VC_BRT_MAGENTA	13
#define VC_BRT_YELLOW	14
#define VC_BRT_WHITE	15
