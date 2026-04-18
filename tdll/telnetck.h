#if !defined(INCL_TELNETCK)
#define INCL_TELNETCK

/*	File: D:\WACKER\tdll\telnetck.h (Created: 26-Nov-1996 by cab)
 *
 *	Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *  Description:
 *      Declares the functions used to implement "telnet checking".
 *      This is UltraTerminal's way of assuring that it is the
 *      default telnet app for Internet Explorer and Netscape Navigator.
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

// IsUltraTerminalDefaultTelnetApp
//
// Returns TRUE if UltraTerminal is the default telnet app
// for Internet Explorer and Netscape Navigator.
//
BOOL IsUltraTerminalDefaultTelnetApp(void);

// AskForDefaultTelnetApp
//
// Returns the value of the "telnet checking" flag. If this is TRUE,
// the app should check whether it is the default telnet app for IE
// and Netscape. If it isn't the default telnet app, then display
// the "Default Telnet App" dialog. The user can disable "telnet
// checking" by checking the "Stop asking me this question" box.
//
BOOL QueryTelnetCheckFlag(void);

// DefaultTelnetAppDlgProc
//
// The dialog procedure for the "Default Telnet App" dialog.
// This dialog asks the user if he/she wants UltraTerminal
// to be the default telnet app for IE and NN. There also is
// a check box to disable this potentially annoying feature.
//
INT_PTR
CALLBACK
DefaultTelnetAppDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wPar, 
    LPARAM lPar);

//	SetTelnetCheckFlag
//
// Sets the "telnet checking" flag which will either turn on or off
// this feature the next time UltraTerminal starts.
int SetTelnetCheckFlag(BOOL fCheck);

//	SetDefaultTelnetApp
//
//	Sets the default telnet application for IE and Netscape to UltraTerminal.
int SetDefaultTelnetApp(void);

#endif
