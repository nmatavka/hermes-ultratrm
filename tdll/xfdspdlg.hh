/* C:\WACKER\TDLL\XFDSPDLG.HH (Created: 10-Jan-1994)
 * Created from:
 * s_r_dlg.h - various stuff used in sends and recieves
 *
 *	Copyright 1994 by Hilgraeve, Inc -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 5/04/01 3:36p $
 */

/*
 * This data is getting moved into the structure in xfer_msc.hh.  It is here
 * only as a transitional device.
 */

struct stReceiveDisplayRecord
	{
	/*
	 * First, we have the bit flags indicating which fields have changed
	 */
	unsigned int 	bChecktype		: 1;
	unsigned int 	bErrorCnt		: 1;
	unsigned int 	bPcktErrCnt		: 1;
	unsigned int 	bLastErrtype	: 1;
	unsigned int 	bVirScan		: 1;
	unsigned int 	bTotalSize		: 1;
	unsigned int 	bTotalSoFar		: 1;
	unsigned int 	bFileSize		: 1;
	unsigned int 	bFileSoFar		: 1;
	unsigned int 	bPacketNumber	: 1;
	unsigned int 	bTotalCnt		: 1;
	unsigned int 	bFileCnt		: 1;
	unsigned int 	bEvent			: 1;
	unsigned int 	bStatus			: 1;
	unsigned int 	bElapsedTime	: 1;
	unsigned int 	bRemainingTime	: 1;
	unsigned int 	bThroughput		: 1;
	unsigned int 	bProtocol		: 1;
	unsigned int 	bMessage		: 1;
	unsigned int 	bOurName		: 1;
	unsigned int 	bTheirName		: 1;

	/*
	 * Then, we have the data fields themselves
	 */
	int 	wChecktype; 				/* Added for XMODEM */
	int 	wErrorCnt;
	int 	wPcktErrCnt;				/* Added for XMODEM */
	int 	wLastErrtype;				/* Added for XMODEM */
	int 	wVirScan;
	long	lTotalSize;
	long	lTotalSoFar;
	long	lFileSize;
	long	lFileSoFar;
	long	lPacketNumber;				/* Added for XMODEM */
	int 	wTotalCnt;
	int 	wFileCnt;
	int 	wEvent;
	int 	wStatus;
	long	lElapsedTime;
	long	lRemainingTime;
	long	lThroughput;
	int 	uProtocol;
	WCHAR	acMessage[80];
	WCHAR	acOurName[256];
	WCHAR	acTheirName[256];
	};

typedef struct stReceiveDisplayRecord	sRD;
typedef sRD FAR *psRD;


