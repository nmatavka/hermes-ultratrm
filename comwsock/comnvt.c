/*  File: D:\WACKER\comwsock\comnvt.c (Created: 14-Feb-1996)
 *
 *  Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 3/16/01 4:48p $
 */


//#define DEBUGSTR

#include <windows.h>
#pragma hdrstop

#include <tdll/stdtyp.h>

#if defined (INCL_WINSOCK)

#include <tdll/session.h>
#include <tdll/com.h>
#include <tdll/comdev.h>
#include <comstd/comstd.hh>
#include "comwsock.hh"
#include <tdll/assert.h>
#include <tdll/ut_text.h>
#include <emu/emu.h>
#include <third_party/suite3270/tn3270_protocol.h>

static PSTOPT LookupOption( ST_STDCOM *hhDriver, ECHAR mc );
static HEMU WinSockQueryEmu(ST_STDCOM *hhDriver);
static VOID WinSockSendIbm3270DeviceType(ST_STDCOM *hhDriver);
static VOID WinSockSendIbm3270Functions(ST_STDCOM *hhDriver);
static VOID WinSockProcessIbm3270TelnetSubneg(ST_STDCOM *hhDriver);
static VOID WinSockProcessStartTlsSubneg(ST_STDCOM *hhDriver);
static VOID WinSockAppendSubnegByte(ST_STDCOM *hhDriver, UCHAR ch);
static VOID WinSockSendStartTlsFollows(ST_STDCOM *hhDriver);
static VOID WinSockProtocolStringCopy(char *pszDest, size_t cchDest,
		const char *pszSrc);
static VOID WinSockWideToProtocolAscii(char *pszDest, int cchDest,
		const WCHAR *pszSrc);


	// This is the "Network Virtual Terminal" emulation, i.e., the code
	// that handles Telnet option negotiations.  WinSockNetworkVirtualTerminal
	// is called to check incoming data to see if there is
	// a Telnet command in there.
	
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  WinSockCreateNVT
 *
 * DESCRIPTION:
 *  This function is called to create the necessary hooks and stuff to create
 *  a Telnet NVT(network virtual terminal).
 *
 * PARAMETERS:
 *  hhDriver    -- private connection handle
 *
 * RETURNS:
 *  Nothing.
 *
 * AUTHOR
 *  mcc 01/09/96 (Ported from NPORT)
 */
VOID WinSockCreateNVT(ST_STDCOM * hhDriver)
	{
	int ix;
    DbgOutStr("WinSockCreateNVT\r\n", 0,0,0,0,0);

	hhDriver->NVTstate = NVT_THRU;

	hhDriver->stMode[ECHO_MODE].option   = TELOPT_ECHO;
    hhDriver->stMode[SGA_MODE].option    = TELOPT_SGA;
    hhDriver->stMode[TTYPE_MODE].option  = TELOPT_TTYPE;
    hhDriver->stMode[BINARY_MODE].option = TELOPT_BINARY;
    hhDriver->stMode[NAWS_MODE].option   = TELOPT_NAWS;
    hhDriver->stMode[EOR_MODE].option    = TELOPT_EOR;
    hhDriver->stMode[TN3270E_MODE].option = TELOPT_TN3270E;
    hhDriver->stMode[STARTTLS_MODE].option = TELOPT_STARTTLS;

	for (ix = 0; ix < MODE_MAX; ++ix)	//jkh 6/18/98
		{
		hhDriver->stMode[ix].us  = hhDriver->stMode[ix].him  = NO;
		hhDriver->stMode[ix].usq = hhDriver->stMode[ix].himq = EMPTY;
		}

	hhDriver->fIbm3270Mode = FALSE;
	hhDriver->fIbm3270E = FALSE;
	hhDriver->fIbm3270EReady = FALSE;
	hhDriver->cbIbm3270Record = 0;
	hhDriver->nSbOption = 0;
	hhDriver->nSbLen = 0;
	hhDriver->fSbIac = FALSE;
	}

	VOID WinSockRequestStartTls(ST_STDCOM *hhDriver)
		{
		PSTOPT pstO;

		if (hhDriver == 0 || !hhDriver->fStartTlsRequested ||
				hhDriver->fStartTlsActive || hhDriver->fImplicitTlsActive)
			return;

		pstO = LookupOption(hhDriver, TELOPT_STARTTLS);
		if (pstO != 0 && pstO->him == NO)
			{
			pstO->him = WANTYES;
			pstO->himq = EMPTY;
			}
		WinSockSendMessage(hhDriver, DO, TELOPT_STARTTLS);
		}

static HEMU WinSockQueryEmu(ST_STDCOM *hhDriver)
	{
	HSESSION hSession;

	if (hhDriver == 0 || hhDriver->hCom == 0)
		return 0;

	if (ComGetSession(hhDriver->hCom, &hSession) != COM_OK || hSession == 0)
		return 0;

	return sessQueryEmuHdl(hSession);
	}

int WinSockIsIbm3270(ST_STDCOM *hhDriver)
	{
	HEMU hEmu;

	if (hhDriver == 0)
		return FALSE;

	if (hhDriver->fIbm3270Mode)
		return TRUE;

	hEmu = WinSockQueryEmu(hhDriver);
	return (hEmu && emuQueryEmulatorId(hEmu) == EMU_IBM3270) ? TRUE : FALSE;
	}

VOID WinSockMaybeStartIbm3270Telnet(ST_STDCOM *hhDriver)
	{
	if (!WinSockIsIbm3270(hhDriver))
		return;

	hhDriver->fIbm3270Mode = TRUE;
	hhDriver->fEscapeFF = FALSE;
	hhDriver->cbIbm3270Record = 0;
	hhDriver->fIbm3270E = FALSE;
	hhDriver->fIbm3270EReady = FALSE;

	WinSockSendMessage(hhDriver, DO, TELOPT_TN3270E);
	WinSockSendMessage(hhDriver, WILL, TELOPT_TN3270E);
	WinSockSendMessage(hhDriver, WILL, TELOPT_TTYPE);
	WinSockSendMessage(hhDriver, DO, TELOPT_BINARY);
	WinSockSendMessage(hhDriver, WILL, TELOPT_BINARY);
	WinSockSendMessage(hhDriver, DO, TELOPT_EOR);
	WinSockSendMessage(hhDriver, WILL, TELOPT_EOR);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  WinSockReleaseNVT
 *
 * DESCRIPTION:
 *  This function is currently a stub
 *
 * PARAMETERS:
 *  hhDriver    -- private connection handle
 *
 * RETURNS:
 *  Nothing.
 */
VOID WinSockReleaseNVT(ST_STDCOM * hhDriver)
	{

	DbgOutStr("WS releaseNVT\r\n", 0,0,0,0,0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  WinSockGotDO
 *
 * DESCRIPTION:
 *  Handles the case of us agreeing that the other side should enable an option
 *
 * PARAMETERS:
 *  hhDriver    --  private handle for this connection driver
 *  pstO        --  Telnet options data structure
 *
 * RETURNS:
 *  nothing
 *
 * AUTHOR:
 *  mcc 01/09/96 (Ported from NPORT)
 */
VOID WinSockGotDO  (ST_STDCOM * hhDriver, const PSTOPT pstO)
	{

	DbgOutStr("Got DO: %lx\r\n", pstO->option, 0,0,0,0);
	if (pstO->option == TELOPT_STARTTLS && !hhDriver->fStartTlsRequested)
		{
		WinSockSendMessage(hhDriver, WONT, pstO->option);
		return;
		}

	switch (pstO->us)
		{
	case NO:
		// We were off, but server want's us on so we agree and respond
		pstO->us = YES;
		WinSockSendMessage(hhDriver, WILL, pstO->option);
		break;

	case YES:
		// Ignore, we're already enabled
		break;

	case WANTNO:
		// This is an error,we had sent a WON'T and they responded with DO
		if (pstO->usq == EMPTY)
			pstO->us = NO;	// leave option as WE wanted it
		else if (pstO->usq == OPPOSITE) // we were going to enable anyway so turn us on
			pstO->us = YES;
		pstO->usq = EMPTY;
		break;

	case WANTYES:
		// They're agreeing with our earlier WILL
		if (pstO->usq == EMPTY)
			{
			pstO->us = YES;	// all done negotiating
			}
		else if (pstO->usq == OPPOSITE)
			{
			// we changed our mind while negotiating, renegotiate for WONT
			pstO->us = WANTNO;
			pstO->usq = EMPTY;
			WinSockSendMessage(hhDriver, WONT, pstO->option);
			}
		break;

	default:
		assert(FALSE);
		break;
		}

	// If the NAWS option was just turned on, we must respond with our terminal size
	// right away. (The WinsockSendNAWS function will check whether the option is now
	// on or off).
	if ( pstO->option == TELOPT_NAWS )
		WinSockSendNAWS( hhDriver );

	if (pstO->option == TELOPT_STARTTLS && pstO->us == YES)
		WinSockSendStartTlsFollows(hhDriver);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
* FUNCTION:
*   WinSockGotWILL
*
* DESCRIPTION:
*   Handles the case of getting a WILL response from the remote Telnet,
*   indicating that an option will be enabled
*
* PARAMETERS:
*  hhDriver --  private handle for this connection driver
*  pstO     --  Telnet options data structure
*
* RETURNS:
*  nothing
*
* AUTHOR:
*  mcc 01/09/96 (Ported from NPORT)
*/
VOID WinSockGotWILL(ST_STDCOM * hhDriver, const PSTOPT pstO)
	{
	DbgOutStr("Got WILL: %lx\r\n", pstO->option, 0,0,0,0);
	if (pstO->option == TELOPT_STARTTLS && !hhDriver->fStartTlsRequested)
		{
		WinSockSendMessage(hhDriver, DONT, pstO->option);
		return;
		}

	switch(pstO->him)
		{
	case NO:
		// He was off but want's to be on so agree and respond
		pstO->him = YES;
		WinSockSendMessage(hhDriver, DO, pstO->option);
		break;

	case YES:
		// He was already on so do nothing
		break;

	case WANTNO:
		// Error: he responded to our DONT with a WILL
		if (pstO->himq == EMPTY)
			pstO->him = NO;
		else if (pstO->himq == OPPOSITE)
			pstO->him = YES;
		pstO->himq = EMPTY;
		break;

	case WANTYES:
		// He responded to our DO with a WILL (life is good!)
		if (pstO->himq == EMPTY)
			{
			pstO->him = YES;
			}
		else if (pstO->himq == OPPOSITE)
			{
			// He agreed to our DO, but we changed our mind -- renegotiate
			pstO->him = WANTNO;
			pstO->himq = EMPTY;
			WinSockSendMessage(hhDriver, DONT, pstO->option);
			}
		break;

	default:
		assert(FALSE);
			break;
			}

	if (pstO->option == TELOPT_STARTTLS && pstO->him == YES)
		WinSockSendStartTlsFollows(hhDriver);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  WinSockGotDONT
 *
 * DESCRIPTION:
 *  Handles the case of getting a DONT option from the remote Telnet,
 *  indicating a request not to implement a particular option
 *
 * PARAMETERS:
 *  hhDriver    Private driver handle
 *  pstO
 *
 * RETURNS:
 *  nothing
 */
VOID WinSockGotDONT(ST_STDCOM * hhDriver, const PSTOPT pstO)
	{
	DbgOutStr("Got DONT: %lx\r\n", pstO->option, 0,0,0,0);
	switch (pstO->us)
		{
	case NO:
		// Got a DONT while we were already off, just ignore
		break;

	case YES:
		// Got a DONT while we were on, agree and respond
		pstO->us = NO;
		WinSockSendMessage(hhDriver, WONT, pstO->option);
		break;

	case WANTNO:
		// He responded to our WONT with a DONT (how appropriate)
		if (pstO->usq == EMPTY)
			{
			pstO->us = NO;
			}
		else if (pstO->usq == OPPOSITE)
			{
			// He agreed to our earlier WONT but we changed our mind
			pstO->us = WANTYES;
			pstO->usq = EMPTY;
			WinSockSendMessage(hhDriver, WILL, pstO->option);
			}
		break;

	case WANTYES:
		// He responded to our WILL with a DONT, so leave it off
		if (pstO->usq == EMPTY)
			{
			pstO->us = NO;
			}
		else if (pstO->usq == OPPOSITE)
			{
			// If he'd agreed to our WILL, we'd have immediately asked for WONT
			// but since he didn't agree, we already got what we wanted
			pstO->us = NO;
			pstO->usq = EMPTY;
			}
		break;

	default:
		assert(FALSE);
		break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
VOID WinSockGotWONT(ST_STDCOM * hhDriver, const PSTOPT pstO)
	{
	DbgOutStr("Got WONT: %lx\r\n", pstO->option, 0,0,0,0);
	switch (pstO->him)
		{
	case NO:
		// Got a WONT while he was already off, just ignore
		break;

	case YES:
		// He wants to change from on to off, agree and respond
		pstO->him = NO;
		WinSockSendMessage(hhDriver, DONT, pstO->option);
		break;

	case WANTNO:
		// He responded to our DONT with a WONT (how agreeable of him)
		if (pstO->himq == EMPTY)
			{
			pstO->him = NO;
			}
		else if (pstO->himq == OPPOSITE)
			{
			// He agreed to our DONT but we changed our mind while waiting
			pstO->him = WANTYES;
			pstO->himq = EMPTY;
			WinSockSendMessage(hhDriver, DO, pstO->option);
			}
		break;

	case WANTYES:
		// He responded to our DO with a WONT -- let the wimp have his way
		if (pstO->himq == EMPTY)
			{
			pstO->him = NO;
			}
		else if (pstO->himq == OPPOSITE)
			{
			// If he'd agreed to our DO, we'd have asked for a DONT so
			// now we're happy anyway
			pstO->him = NO;
			pstO->himq = EMPTY;
			}
		break;

	default:
		assert(FALSE);
		break;
		}
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  WinSockNetworkVirtualTerminal
 *
 * DESCRIPTION:
 *  called from CLoop to handle Telnet option negotiation
 *
 * PARAMETERS:
 *	mc		The current character being processed
 *	pD		Pointer to Winsock connection driver private handle
 *
 * RETURNS:
 *	NVT_DISCARD		if mc is to be discarded
 *	NVT_KEEP			if mc is to be processed further
 *
 * AUTHOR
 *	mcc  01/09/96 (mostly from NPORT)
 */
int FAR PASCAL WinSockNetworkVirtualTerminal(ECHAR mc, void *pD)
	{
	ST_STDCOM * hhDriver = (ST_STDCOM *)pD;
#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
    STEMUSET stEmuSet;
	char achTermType[EMU_MAX_TELNETID];
#else
	int nTtype;
#endif
	LPSTR pszPtr;
	UCHAR acTerm[320];
	HEMU  hEmu;
	HSESSION hSession;
	PSTOPT pstTelnetOpt;

	assert(hhDriver);
	
	//DbgOutStr("NVT %d %c(0x%x = %d)\n", hhDriver->NVTstate,
		// ((mc == 0)? ' ': mc), mc, mc,0);

	switch (hhDriver->NVTstate)
		{
	case NVT_THRU:
		if (mc == IAC)
			{
			hhDriver->NVTstate = NVT_IAC;
			return NVT_DISCARD ;
			}
		return NVT_KEEP ;

	case NVT_IAC:
		switch (mc)
			{
		case IAC:
			hhDriver->NVTstate = NVT_THRU;       // Got a doubled IAC, keep one
			return  NVT_KEEP ;
		case DONT:
			hhDriver->NVTstate = NVT_DONT;
			return  NVT_DISCARD ;
		case DO:
			hhDriver->NVTstate = NVT_DO;
			return  NVT_DISCARD ;
		case WONT:
			hhDriver->NVTstate = NVT_WONT;
			return  NVT_DISCARD ;
		case WILL:
			hhDriver->NVTstate = NVT_WILL;
			return  NVT_DISCARD ;
		case SB:
			hhDriver->NVTstate = NVT_SB;
			return  NVT_DISCARD ;
		case EOR:
			hhDriver->NVTstate = NVT_THRU;
			return WinSockIsIbm3270(hhDriver) ? NVT_EOR : NVT_DISCARD;
		case GA:
		case EL:
		case EC:
		case AYT:
		case AO:
		case IP:
		case BREAK:
		case DM:
		case SE:
			//MessageBeep((UINT)-1);
			hhDriver->NVTstate = NVT_THRU;
			return  NVT_DISCARD ;	// ignore all these
		case NOP:
		default:
			hhDriver->NVTstate = NVT_THRU;
			return NVT_KEEP;
			}

	case NVT_WILL:
		pstTelnetOpt = LookupOption( hhDriver, mc );
		if ( pstTelnetOpt )
			WinSockGotWILL( hhDriver, pstTelnetOpt ); // We support the option, negotiate
		else
			WinSockSendMessage( hhDriver, DONT, mc ); // We don't support it, decline	

		hhDriver->NVTstate = NVT_THRU;
		return  NVT_DISCARD ;

	case NVT_WONT:
		pstTelnetOpt = LookupOption( hhDriver, mc );
		if ( pstTelnetOpt )
			WinSockGotWONT( hhDriver, pstTelnetOpt ); // We support the option, negotiate

		// Since we don't support this option, it is always off, and we never respond
		// when the other side tries to set a state that already exists

		hhDriver->NVTstate = NVT_THRU;
		return  NVT_DISCARD ;

	case NVT_DO:
		pstTelnetOpt = LookupOption( hhDriver, mc );
		if ( pstTelnetOpt )
			WinSockGotDO( hhDriver, pstTelnetOpt ); // We support the option, negotiate
		else
			WinSockSendMessage( hhDriver, WONT, mc ); // We don't support it, decline

		hhDriver->NVTstate = NVT_THRU;
		return  NVT_DISCARD ;

	case NVT_DONT:
		pstTelnetOpt = LookupOption( hhDriver, mc );
		if ( pstTelnetOpt )
			WinSockGotDONT( hhDriver, pstTelnetOpt ); // We support the option, negotiate

		// Since we don't support this option, it is always off, and we never respond
		// when the other side tries to set a state that already exists

		hhDriver->NVTstate = NVT_THRU;
		return  NVT_DISCARD ;

		case NVT_SB:
			/* At this time we only handle one sub-negotiation */
			switch (mc)
				{
			case TELOPT_TTYPE:
			hhDriver->NVTstate = NVT_SB_TT;
			return  NVT_DISCARD ;
		case TELOPT_TN3270E:
			hhDriver->NVTstate = NVT_SB_TN3270E;
			hhDriver->nSbOption = TELOPT_TN3270E;
				hhDriver->nSbLen = 0;
				hhDriver->fSbIac = FALSE;
				return NVT_DISCARD;
			case TELOPT_STARTTLS:
				hhDriver->NVTstate = NVT_SB_STARTTLS;
				hhDriver->nSbOption = TELOPT_STARTTLS;
				hhDriver->nSbLen = 0;
				hhDriver->fSbIac = FALSE;
				return NVT_DISCARD;
			default:
				break;
				}
		hhDriver->NVTstate = NVT_THRU;
		return NVT_KEEP;

	case NVT_SB_TT:
		switch (mc)
			{
		case TELQUAL_SEND:
			hhDriver->NVTstate = NVT_SB_TT_S;
			return NVT_DISCARD ;
		default:
			break;
			}
		hhDriver->NVTstate = NVT_THRU;
		return NVT_KEEP;

	case NVT_SB_TT_S:
		switch (mc)
			{
		case IAC:
			hhDriver->NVTstate = NVT_SB_TT_S_I;
			return NVT_DISCARD ;
		default:
			break;
			}
		hhDriver->NVTstate = NVT_THRU;
		return NVT_KEEP;

	case NVT_SB_TT_S_I:
		switch (mc)
			{
		case SE:
			memset(acTerm, 0, sizeof(acTerm));
			pszPtr = (char *)acTerm;
			*pszPtr++ = (UCHAR)IAC;
			*pszPtr++ = (UCHAR)SB;
			*pszPtr++ = (UCHAR)TELOPT_TTYPE;
			*pszPtr++ = (UCHAR)TELQUAL_IS;

			ComGetSession(hhDriver->hCom, &hSession);
			assert(hSession);

			hEmu = sessQueryEmuHdl(hSession);
			assert(hEmu);

#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
            // The telnet terminal ids are no longer hard-coded. We
            // are now using the terminal id that is supplied by the
            // user in the "Settings" properties page. - cab:11/18/96
            //
            emuQuerySettings(hEmu, &stEmuSet);
            if (stEmuSet.nEmuId == EMU_IBM3270 && stEmuSet.acTelnetId[0] == L'\0')
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"IBM-3278-2-E");
            else
				{
				WinSockWideToProtocolAscii(achTermType, sizeof(achTermType),
						stEmuSet.acTelnetId);
				WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						achTermType[0] ? achTermType : "DEC-VT100");
				}
#else
			nTtype = emuQueryEmulatorId(hEmu);
			switch (nTtype)
				{
			case EMU_ANSI:
				WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"ANSI");
				break;
			case EMU_TTY:
				WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"TELETYPE-33");
				break;
			case EMU_VT52:
				WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"DEC-VT52");
				break;
			case EMU_VT100:
                // strcpy(pszPtr, "VT100");
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"DEC-VT100");
				break;
#if defined(INCL_VT220)
			case EMU_VT220:
                // strcpy(pszPtr, "VT220");
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"DEC-VT220");
				break;
#endif
#if defined(INCL_VT320)
			case EMU_VT220:
                // strcpy(pszPtr, "VT320");
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"DEC-VT320");
				break;
#endif

#if defined(INCL_VT100PLUS)
			case EMU_VT100PLUS:
                // strcpy(pszPtr, "VT100");
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"DEC-VT100");
				break;
#endif

#if defined(INCL_VTUTF8)
			case EMU_VTUTF8:
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"VT-UTF8");
				break;
#endif

			default:
                WinSockProtocolStringCopy(pszPtr,
						sizeof(acTerm) - (size_t)(pszPtr - (char *)acTerm),
						"DEC-VT100"); // "UNKNOWN");
				break;
				}
#endif

			DbgOutStr("NVT: Terminal=%s", pszPtr, 0,0,0,0);
			pszPtr = pszPtr + lstrlenA(pszPtr);
			*pszPtr++ = (UCHAR)IAC;
			*pszPtr++ = (UCHAR)SE;

			WinSockSendBuffer(hhDriver,
				(INT)(pszPtr - (char *)acTerm),
				acTerm);
			hhDriver->NVTstate = NVT_THRU;
			return NVT_DISCARD ;
		default:
			break;
			}
		hhDriver->NVTstate = NVT_THRU;
		return NVT_KEEP;

	case NVT_SB_TN3270E:
		if (hhDriver->fSbIac)
			{
			hhDriver->fSbIac = FALSE;
			if (mc == SE)
				{
				WinSockProcessIbm3270TelnetSubneg(hhDriver);
				hhDriver->NVTstate = NVT_THRU;
				return NVT_DISCARD;
				}
			if (mc == IAC)
				{
				WinSockAppendSubnegByte(hhDriver, (UCHAR)IAC);
				return NVT_DISCARD;
				}
			hhDriver->NVTstate = NVT_THRU;
			return NVT_DISCARD;
			}

		if (mc == IAC)
			{
			hhDriver->fSbIac = TRUE;
			return NVT_DISCARD;
			}

			WinSockAppendSubnegByte(hhDriver, (UCHAR)mc);
			return NVT_DISCARD;

		case NVT_SB_STARTTLS:
			if (hhDriver->fSbIac)
				{
				hhDriver->fSbIac = FALSE;
				if (mc == SE)
					{
					WinSockProcessStartTlsSubneg(hhDriver);
					hhDriver->NVTstate = NVT_THRU;
					return NVT_DISCARD;
					}
				if (mc == IAC)
					{
					WinSockAppendSubnegByte(hhDriver, (UCHAR)IAC);
					return NVT_DISCARD;
					}
				hhDriver->NVTstate = NVT_THRU;
				return NVT_DISCARD;
				}

			if (mc == IAC)
				{
				hhDriver->fSbIac = TRUE;
				return NVT_DISCARD;
				}

			WinSockAppendSubnegByte(hhDriver, (UCHAR)mc);
			return NVT_DISCARD;

		default:
			hhDriver->NVTstate = NVT_THRU;
		return NVT_KEEP;
		}

	}

static VOID WinSockAppendSubnegByte(ST_STDCOM *hhDriver, UCHAR ch)
	{
	if (hhDriver->nSbLen < (int)sizeof(hhDriver->abSbBuf))
		hhDriver->abSbBuf[hhDriver->nSbLen++] = ch;
	}

static VOID WinSockSendStartTlsFollows(ST_STDCOM *hhDriver)
	{
	static const UCHAR acStartTlsFollows[] =
		{
		(UCHAR)IAC, (UCHAR)SB, (UCHAR)TELOPT_STARTTLS,
		(UCHAR)TLS_STARTTLS_FOLLOWS, (UCHAR)IAC, (UCHAR)SE
		};

	if (hhDriver == 0 || !hhDriver->fStartTlsRequested ||
			hhDriver->fStartTlsActive || hhDriver->fImplicitTlsActive)
		return;

	WinSockSendBuffer(hhDriver, (INT)sizeof(acStartTlsFollows),
			acStartTlsFollows);
	if (WsckStartTls(hhDriver) < 0)
		{
		hhDriver->fStartTlsRequested = 0;
		hhDriver->fImplicitTlsRequested = 0;
		}
	else
		{
		WinSockMaybeStartIbm3270Telnet(hhDriver);
		}
	}

static VOID WinSockProcessStartTlsSubneg(ST_STDCOM *hhDriver)
	{
	if (hhDriver == 0 || hhDriver->nSbLen < 1)
		return;

	if (!hhDriver->fStartTlsRequested ||
			hhDriver->abSbBuf[0] != TLS_STARTTLS_FOLLOWS ||
			hhDriver->fStartTlsActive || hhDriver->fImplicitTlsActive)
		return;

	if (WsckStartTls(hhDriver) < 0)
		{
		hhDriver->fStartTlsRequested = 0;
		hhDriver->fImplicitTlsRequested = 0;
		}
	else
		{
		WinSockMaybeStartIbm3270Telnet(hhDriver);
		}
	}

static VOID WinSockProcessIbm3270TelnetSubneg(ST_STDCOM *hhDriver)
	{
	HEMU hEmu;
	UCHAR *buf;
	int len;

	buf = hhDriver->abSbBuf;
	len = hhDriver->nSbLen;

	if (len < 1)
		return;

	if (len >= 2 && buf[0] == UT_TN3270E_OP_SEND &&
			buf[1] == UT_TN3270E_OP_DEVICE_TYPE)
		{
		WinSockSendIbm3270DeviceType(hhDriver);
		return;
		}

	if (len >= 2 && buf[0] == UT_TN3270E_OP_DEVICE_TYPE &&
			buf[1] == UT_TN3270E_OP_IS)
		{
		hhDriver->fIbm3270E = TRUE;
		hhDriver->fIbm3270EReady = TRUE;
		hEmu = WinSockQueryEmu(hhDriver);
		if (hEmu)
			emuIbm3270SetTelnetMode(hEmu, TRUE);
		return;
		}

	if (len >= 2 && buf[0] == UT_TN3270E_OP_FUNCTIONS &&
			buf[1] == UT_TN3270E_OP_REQUEST)
		{
		WinSockSendIbm3270Functions(hhDriver);
		return;
		}

	if (len >= 2 && buf[0] == UT_TN3270E_OP_FUNCTIONS &&
			buf[1] == UT_TN3270E_OP_IS)
		{
		hhDriver->fIbm3270EReady = TRUE;
		return;
		}

	if (buf[0] == UT_TN3270E_OP_REJECT)
		{
		hhDriver->fIbm3270E = FALSE;
		hhDriver->fIbm3270EReady = FALSE;
		hEmu = WinSockQueryEmu(hhDriver);
		if (hEmu)
			emuIbm3270SetTelnetMode(hEmu, FALSE);
		}
	}

static VOID WinSockSendIbm3270DeviceType(ST_STDCOM *hhDriver)
	{
	UCHAR out[256];
	char term[EMU_IBM3270_MAX_LUNAME];
	char lu[EMU_IBM3270_MAX_LUNAME];
	char telnet_id[EMU_MAX_TELNETID];
	STEMUSET stEmuSet;
	HEMU hEmu;
	int pos;
	int i;

	memset(&stEmuSet, 0, sizeof(stEmuSet));
	hEmu = WinSockQueryEmu(hhDriver);
	if (hEmu)
		emuQuerySettings(hEmu, &stEmuSet);

	WinSockWideToProtocolAscii(term, sizeof(term),
			stEmuSet.acIbm3270DeviceName);
	if (term[0] == '\0')
		{
		WinSockWideToProtocolAscii(telnet_id, sizeof(telnet_id),
				stEmuSet.acTelnetId);
		if (telnet_id[0] != '\0')
			{
			WinSockProtocolStringCopy(term, sizeof(term), telnet_id);
			}
		else
			WinSockProtocolStringCopy(term, sizeof(term), "IBM-3278-2-E");
		}
	WinSockWideToProtocolAscii(lu, sizeof(lu), stEmuSet.acIbm3270LuName);

	pos = 0;
	out[pos++] = (UCHAR)IAC;
	out[pos++] = (UCHAR)SB;
	out[pos++] = (UCHAR)TELOPT_TN3270E;
	out[pos++] = (UCHAR)UT_TN3270E_OP_DEVICE_TYPE;
	out[pos++] = (UCHAR)UT_TN3270E_OP_REQUEST;
	for (i = 0; term[i] != '\0' && pos < (int)sizeof(out) - 8; ++i)
		out[pos++] = (UCHAR)term[i];
	if (lu[0] != '\0' && pos < (int)sizeof(out) - 8)
		{
		out[pos++] = (UCHAR)UT_TN3270E_OP_CONNECT;
		for (i = 0; lu[i] != '\0' && pos < (int)sizeof(out) - 3; ++i)
			out[pos++] = (UCHAR)lu[i];
		}
	out[pos++] = (UCHAR)IAC;
	out[pos++] = (UCHAR)SE;

	WinSockSendBuffer(hhDriver, pos, out);
	}

static VOID WinSockWideToProtocolAscii(char *pszDest, int cchDest,
		const WCHAR *pszSrc)
	{
	int i;
	int j;
	WCHAR ch;

	if (pszDest == NULL || cchDest <= 0)
		return;

	pszDest[0] = '\0';
	if (pszSrc == NULL)
		return;

	for (i = 0, j = 0; pszSrc[i] != L'\0' && j < cchDest - 1; ++i)
		{
		ch = pszSrc[i];
		if (ch >= L' ' && ch <= L'~')
			pszDest[j++] = (char)ch;
		else if (ch > L'~')
			pszDest[j++] = '?';
		}
	pszDest[j] = '\0';
	}

static VOID WinSockProtocolStringCopy(char *pszDest, size_t cchDest,
		const char *pszSrc)
	{
	size_t i;

	if (pszDest == NULL || cchDest == 0)
		return;

	if (pszSrc == NULL)
		pszSrc = "";

	for (i = 0; i + 1 < cchDest && pszSrc[i] != '\0'; ++i)
		pszDest[i] = pszSrc[i];
	pszDest[i] = '\0';
	}

static VOID WinSockSendIbm3270Functions(ST_STDCOM *hhDriver)
	{
	UCHAR out[18];
	int pos;

	pos = 0;
	out[pos++] = (UCHAR)IAC;
	out[pos++] = (UCHAR)SB;
	out[pos++] = (UCHAR)TELOPT_TN3270E;
	out[pos++] = (UCHAR)UT_TN3270E_OP_FUNCTIONS;
	out[pos++] = (UCHAR)UT_TN3270E_OP_IS;
	out[pos++] = (UCHAR)UT_TN3270E_FUNC_BIND_IMAGE;
	out[pos++] = (UCHAR)UT_TN3270E_FUNC_DATA_STREAM_CTL;
	out[pos++] = (UCHAR)UT_TN3270E_FUNC_RESPONSES;
	out[pos++] = (UCHAR)UT_TN3270E_FUNC_SCS_CTL_CODES;
	out[pos++] = (UCHAR)UT_TN3270E_FUNC_SYSREQ;
	out[pos++] = (UCHAR)UT_TN3270E_FUNC_CONTENTION_RESOLUTION;
	out[pos++] = (UCHAR)IAC;
	out[pos++] = (UCHAR)SE;

	WinSockSendBuffer(hhDriver, pos, out);
	}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FUNCTION:    WinSockSendNAWS
//
// DESCRIPTION: Sends our terminal dimensions according to the Telnet NAWS option
//              specification. NAWS stands for Negotiate About Window Size. It is
//              defined in RFC 1073, "Telnet Window Size Option". If a telnet server
//              enables this capability by sending us an IAC DO NAWS sequence, we
//              will agree to it by responding with IAC WILL NAWS and then sending
//              the number of rows and columns in a sub-option negotiation sequence
//              as implemented here. We also send the sub-option sequence whenever
//              our terminal size changes.
//
// ARGUMENTS:   hhDriver -- pointer to our com driver
//
// RETURNS:     void
//
// AUTHOR:      John Hile, 6/17/98
//
VOID WinSockSendNAWS( ST_STDCOM *hhDriver )
	{
	HEMU	 hEmu;
	HSESSION hSession;
	int		 iRows;
	int		 iCols;
	UCHAR    achOutput[9];	// exact size

	// We've been asked to send our terminal size to the server. We're only
	// allowed to do so if we have successfully enabled the NAWS option with
	// the server.
	if ( hhDriver->stMode[NAWS_MODE].us == YES)
		{
		// OK, option has been turned on. Send
		//  "IAC SB NAWS WIDTH[1] WIDTH[0] HEIGHT[1] HEIGHT[0] IAC SE" to server

		// Get actual terminal size (not menu settings) from emulator
		ComGetSession(hhDriver->hCom, &hSession);
		assert(hSession);

		hEmu = sessQueryEmuHdl(hSession);
		assert(hEmu);
		emuQueryRowsCols( hEmu, &iRows, &iCols );
		achOutput[0] = (UCHAR)IAC;
		achOutput[1] = (UCHAR)SB;
		achOutput[2] = (UCHAR)TELOPT_NAWS;
		achOutput[3] = (UCHAR)(iCols / 0xFF);
		achOutput[4] = (UCHAR)(iCols % 0xFF);
		achOutput[5] = (UCHAR)(iRows / 0xFF);
		achOutput[6] = (UCHAR)(iRows % 0xFF);
		achOutput[7] = (UCHAR)IAC;
		achOutput[8] = (UCHAR)SE;

		WinSockSendBuffer(hhDriver, sizeof(achOutput), achOutput);
		}
	}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FUNCTION:    LookupTelnetOption
//
// DESCRIPTION: Searches our table of telnet option management structures to
//              see whether we support the option coded by character mc.
//
// ARGUMENTS:   hhDriver -- our comm driver handle
//              mc       -- the character that defines the option we're looking up
//
// RETURNS:     Pointer to the option management structure if found or NULL otherwise
//
// AUTHOR:      John Hile, 6/17/98
//
static PSTOPT LookupOption( ST_STDCOM *hhDriver, ECHAR mc )
	{
	int ix;

	for (ix = 0; ix < MODE_MAX; ix++)
	if (hhDriver->stMode[ix].option == mc)
		return &hhDriver->stMode[ix];

	return (PSTOPT)0;
	}


#endif
