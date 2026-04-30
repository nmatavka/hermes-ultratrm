/*	File: D:\WACKER\tdll\xfer_msc.hh (Created: 30-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 2/16/01 4:41p $
 */

 #include "tdll/sess_ids.h"

/*
 * All the structures in this structure are defined in other modules.  They
 * all conform to the single rule that the first element of the structure is
 * an integer that contains the size of the structure.
 */

struct stSizeType
	{
	int nSize;
	};

typedef struct stSizeType SZ_TYPE;

struct stXferData
	{

	HSESSION hSession;

	/* This holds the generic transfer parameters */
	SZ_TYPE		*xfer_old_params;	/* for conditional save */
	SZ_TYPE		*xfer_params;		/* the parameters currently in use */

	/* TODO: add stuff for conditional save */

	/* This holds the protocol specific parameters */
	SZ_TYPE		*xfer_proto_params[SFID_PROTO_PARAMS_END - SFID_PROTO_PARAMS];

#define XFER_NONE       0
#define	XFER_SEND		1
#define	XFER_RECV		2
	int nDirection;

#define	XFER_ABORT		1
#define	XFER_SKIP		2
	int nUserCancel;				/* User canceled the transfer */

	int nCarrierLost;				/* Carrier has been lost */


	/* This holds the stuff for the send list */
	int nSendListCount;				/* How many files so far */
	WCHAR **acSendNames;			/* Pointer to list block */

	VOID *pXferStuff;				/* Set by the other side for storage */

	/*
	 * This block of stuff is used by the transfer displays.
	 */
	HWND hwndXfrDisplay;			/* handle of the display window */

	int nLgSingleTemplate;			/* template ID for single file transfer */
	int nLgMultiTemplate;			/* template ID for multiple file xfer */

#if FALSE
	/* Removed and switched to integers */
	LPCSTR pszLgSingleTemplate;		/* template for single file transfer */
	LPCSTR pszLgMultiTemplate;		/* template for multiple file transfer */
#if FALSE
	/* Size change is not supported in Lower Wacker */
	LPWSTR pszSmSingleTemplate;
	LPWSTR pszSmMultiTemplate;
#endif
#endif

	int nStatusBase;				/* start of status ID list */
	int nEventBase;					/* start of event ID list */

	int nOldBps;					/* Saved copy of the following */
	int nBps;						/* TRUE to display as BPS vs CPS */

	int nExpanded;					/* Set if we have already expanded */

	int nCancel;					/* the ever popular cancel option */
	int nSkip;						/* TRUE if we want to skip this file */

	int	nPerCent;					/* Percent done, if we know it */

	int nClose;						/* The transfer finished */
	int nCloseStatus;				/* The closing status */
	/*
	 * First, we have the bit flags indicating which fields have changed
	 */
	unsigned int		bChecktype		: 1;
	unsigned int		bErrorCnt		: 1;
	unsigned int		bPcktErrCnt		: 1;
	unsigned int		bLastErrtype	: 1;
	unsigned int		bTotalSize		: 1;
	unsigned int		bTotalSoFar		: 1;
	unsigned int		bFileSize		: 1;
	unsigned int		bFileSoFar		: 1;
	unsigned int		bPacketNumber	: 1;
	unsigned int		bTotalCnt		: 1;
	unsigned int		bFileCnt		: 1;
	unsigned int		bEvent			: 1;
	unsigned int		bStatus			: 1;
	unsigned int		bElapsedTime	: 1;
	unsigned int		bRemainingTime	: 1;
	unsigned int		bThroughput		: 1;
	unsigned int		bProtocol		: 1;
	unsigned int		bMessage		: 1;
	unsigned int		bOurName		: 1;
	unsigned int		bTheirName		: 1;

	/*
	 * Then, we have the data fields themselves
	 */
	int		wChecktype; 				/* Added for XMODEM */
	int		wErrorCnt;
	int		wPcktErrCnt;				/* Added for XMODEM */
	int		wLastErrtype;				/* Added for XMODEM */
	long	lTotalSize;
	long	lTotalSoFar;
	long	lFileSize;
	long	lFileSoFar;
	long	lPacketNumber;				/* Added for XMODEM */
	int		wTotalCnt;
	int		wFileCnt;
	int		wEvent;
	int		wStatus;
	long	lElapsedTime;
	long	lRemainingTime;
	long	lThroughput;
	int		uProtocol;
	WCHAR	acMessage[80];
	WCHAR	acOurName[256];
	WCHAR	acTheirName[256];
	/*
	 * End of the transfer display data !!
	 */

	};

typedef struct stXferData XD_TYPE;

