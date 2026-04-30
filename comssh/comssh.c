/*  File: comssh.c
 *
 *  UltraTerminal SSH transport driver.
 *
 *  This driver owns the in-process OpenSSH runtime boundary.  It must not
 *  spawn an external SSH client; all transport bytes flow through utssh.
 */

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <tdll/stdtyp.h>
#include <tdll/session.h>
#include <tdll/com.h>
#include <tdll/comdev.h>
#include <tdll/com.hh>
#include <tdll/cnct.h>
#include <comstd/comstd.hh>
#include <comwsock/comwsock.hh>
#include <emu/emu.h>
#include <third_party/suite3270/tn3270_protocol.h>
#include <openssh_ultra.h>

#if defined(INCL_SSH)

#define SSH_RX_QUEUE_SIZE       131072
#define SSH_READ_CHUNK          4096
#define SSH_STATUS_CHARS        256

typedef struct stSshCom ST_SSHCOM;

struct stSshCom
    {
    HCOM hCom;
    CRITICAL_SECTION cs;
    ST_SSHCOM *pstNext;

    UTSSH_SESSION *pstSession;

    int fActivated;
    int fConnected;
    int fSending;
    int nMode;
    int iSshPort;
    int iTargetPort;
    int fAgent;

    WCHAR achSshHost[128];
    WCHAR achSshUser[128];
    WCHAR achSshConfigPath[MAX_PATH];
    WCHAR achSshKnownHostsPath[MAX_PATH];
    WCHAR achSshIdentityFiles[512];
    WCHAR achSshTargetHost[128];
    WCHAR achSshProxy[512];
    WCHAR achSshForwardings[512];
    WCHAR achStatus[SSH_STATUS_CHARS];
    WCHAR achError[SSH_STATUS_CHARS];

    BYTE *pbRx;
    int cbRxSize;
    int cbRxHead;
    int cbRxTail;
    int cbRxUsed;
    BYTE abRead[SSH_READ_CHUNK];

    int nvtState;
    int nSbOption;
    int nSbLen;
    int fSbIac;
    BYTE abSbBuf[512];
    int fIbm3270Started;
    int fIbm3270E;
    int cbIbm3270Record;
    BYTE abIbm3270Record[8192];
    };

static int WINAPI SshPortActivate(void *pvPrivate, WCHAR *pszPortName,
        DWORD_PTR dwMediaHdl);
static int WINAPI SshPortDeactivate(void *pvPrivate);
static int WINAPI SshPortConnected(void *pvPrivate);
static int WINAPI SshRcvRefill(void *pvPrivate);
static int WINAPI SshRcvClear(void *pvPrivate);
static int WINAPI SshSndBufrSend(void *pvPrivate, void *pvBufr, int nCount);
static int WINAPI SshSndBufrIsBusy(void *pvPrivate);
static int WINAPI SshSndBufrClear(void *pvPrivate);
static int WINAPI SshSndBufrQuery(void *pvPrivate, unsigned *pafStatus,
        long *plHandshakeDelay);
static int WINAPI SshDeviceSpecial(void *pvPrivate,
        const WCHAR *pszInstructions, WCHAR *pszResults, int nBufrSize);
static int WINAPI SshPortConfigure(void *pvPrivate);

static ST_SSHCOM *SshFindOrCreate(HCOM hCom);
static ST_SSHCOM *SshFromPrivate(void *pvPrivate);
static void SshStopProcess(ST_SSHCOM *pstSsh);
static void SshSetStatus(ST_SSHCOM *pstSsh, const WCHAR *pszStatus);
static void SshSetConnectedStatus(ST_SSHCOM *pstSsh);
static void SshSetError(ST_SSHCOM *pstSsh, const WCHAR *pszError);
static int SshQueueBytes(ST_SSHCOM *pstSsh, const BYTE *pbData, int cbData);
static int SshPopBytes(ST_SSHCOM *pstSsh, BYTE *pbData, int cbData);
static int SshSendInternal(ST_SSHCOM *pstSsh, const BYTE *pbData, int cbData);
static int SshGetEmulatorId(ST_SSHCOM *pstSsh);
static HEMU SshGetEmu(ST_SSHCOM *pstSsh);
static int SshProcessTelnetBytes(ST_SSHCOM *pstSsh, const BYTE *pbIn,
        int cbIn, BYTE *pbOut, int cbOut, int fIbm3270);
static void SshMaybeStartIbm3270(ST_SSHCOM *pstSsh);
static void SshSendTelnetCommand(ST_SSHCOM *pstSsh, int nCommand, int nOption);
static void SshSendTerminalType(ST_SSHCOM *pstSsh);
static void SshSendNAWS(ST_SSHCOM *pstSsh);
static void SshAppendSubnegByte(ST_SSHCOM *pstSsh, BYTE b);
static void SshProcessIbm3270Subneg(ST_SSHCOM *pstSsh);
static void SshSendIbm3270DeviceType(ST_SSHCOM *pstSsh);
static void SshSendIbm3270Functions(ST_SSHCOM *pstSsh);
static void SshDeliverIbm3270Record(ST_SSHCOM *pstSsh);
static void SshWideToProtocolAscii(char *pszDest, int cchDest,
        const WCHAR *pszSrc);
static int SshOptionSupportedLocal(int nOption, int fIbm3270);
static int SshOptionSupportedRemote(int nOption, int fIbm3270);
static void SshCopy(WCHAR *pszDest, int cchDest, const WCHAR *pszSrc);
static const WCHAR *SshSkipSpaces(const WCHAR *psz);
static int SshStartsWith(const WCHAR *psz, const WCHAR *pszPrefix);
static int SshSetInt(const WCHAR *psz);

static CRITICAL_SECTION g_csSshList;
static volatile LONG g_lSshListInit = 0;
static ST_SSHCOM *g_pstSshList = 0;

static void SshInitList(void)
    {
    LONG lPrev;

    lPrev = InterlockedCompareExchange((LONG *)&g_lSshListInit, 1, 0);
    if (lPrev == 0)
        {
        InitializeCriticalSection(&g_csSshList);
        InterlockedExchange((LONG *)&g_lSshListInit, 2);
        return;
        }

    while (g_lSshListInit != 2)
        Sleep(0);
    }

int ComLoadSshDriver(HCOM pstCom)
    {
    if (!pstCom)
        return COM_FAILED;

    if (SshFindOrCreate(pstCom) == 0)
        return COM_NOT_ENOUGH_MEMORY;

    pstCom->pfPortActivate = SshPortActivate;
    pstCom->pfPortDeactivate = SshPortDeactivate;
    pstCom->pfPortConnected = SshPortConnected;
    pstCom->pfRcvRefill = SshRcvRefill;
    pstCom->pfRcvClear = SshRcvClear;
    pstCom->pfSndBufrSend = SshSndBufrSend;
    pstCom->pfSndBufrIsBusy = SshSndBufrIsBusy;
    pstCom->pfSndBufrClear = SshSndBufrClear;
    pstCom->pfSndBufrQuery = SshSndBufrQuery;
    pstCom->pfDeviceSpecial = SshDeviceSpecial;
    pstCom->pfPortConfigure = SshPortConfigure;
    return COM_OK;
    }

static ST_SSHCOM *SshFindOrCreate(HCOM hCom)
    {
    ST_SSHCOM *pstWalk;
    ST_SSHCOM *pstNew;

    if (hCom == 0)
        return 0;

    SshInitList();
    EnterCriticalSection(&g_csSshList);
    for (pstWalk = g_pstSshList; pstWalk != 0; pstWalk = pstWalk->pstNext)
        {
        if (pstWalk->hCom == hCom)
            {
            LeaveCriticalSection(&g_csSshList);
            return pstWalk;
            }
        }

    pstNew = (ST_SSHCOM *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(ST_SSHCOM));
    if (pstNew != 0)
        {
        pstNew->pbRx = (BYTE *)HeapAlloc(GetProcessHeap(), 0,
                SSH_RX_QUEUE_SIZE);
        if (pstNew->pbRx == 0)
            {
            HeapFree(GetProcessHeap(), 0, pstNew);
            pstNew = 0;
            }
        }

    if (pstNew != 0)
        {
        InitializeCriticalSection(&pstNew->cs);
        pstNew->hCom = hCom;
        pstNew->cbRxSize = SSH_RX_QUEUE_SIZE;
        pstNew->nMode = CNCT_TRANSPORT_SSH_TUNNEL;
        pstNew->iSshPort = 22;
        pstNew->iTargetPort = 23;
        pstNew->fAgent = TRUE;
        pstNew->nvtState = NVT_THRU;
        SshSetStatus(pstNew, L"SSH idle");
        pstNew->pstNext = g_pstSshList;
        g_pstSshList = pstNew;
        }

    LeaveCriticalSection(&g_csSshList);
    return pstNew;
    }

static ST_SSHCOM *SshFromPrivate(void *pvPrivate)
    {
    ST_STDCOM *pstStd;

    if (pvPrivate == 0)
        return 0;

    pstStd = (ST_STDCOM *)pvPrivate;
    return SshFindOrCreate(pstStd->hCom);
    }

static void SshSetStatus(ST_SSHCOM *pstSsh, const WCHAR *pszStatus)
    {
    if (pstSsh == 0)
        return;
    SshCopy(pstSsh->achStatus,
            sizeof(pstSsh->achStatus) / sizeof(WCHAR), pszStatus);
    }

static void SshSetConnectedStatus(ST_SSHCOM *pstSsh)
    {
    const WCHAR *pszProtocol;
    WCHAR achStatus[SSH_STATUS_CHARS];

    if (pstSsh == 0)
        return;

    pszProtocol = emuQueryIbmProtocolName(SshGetEmulatorId(pstSsh),
            pstSsh->nMode);
    if (pszProtocol == NULL)
        {
        SshSetStatus(pstSsh, L"SSH connected");
        return;
        }

    SshCopy(achStatus, SSH_STATUS_CHARS, pszProtocol);
    SshCopy(achStatus + lstrlenW(achStatus),
            SSH_STATUS_CHARS - lstrlenW(achStatus), L" connected");
    SshSetStatus(pstSsh, achStatus);
    }

static void SshSetError(ST_SSHCOM *pstSsh, const WCHAR *pszError)
    {
    if (pstSsh == 0)
        return;
    SshCopy(pstSsh->achError,
            sizeof(pstSsh->achError) / sizeof(WCHAR), pszError);
    SshSetStatus(pstSsh, pszError);
    }

static int WINAPI SshPortActivate(void *pvPrivate, WCHAR *pszPortName,
        DWORD_PTR dwMediaHdl)
    {
    ST_SSHCOM *pstSsh;
    UTSSH_OPTIONS stOptions;
    WCHAR achStatus[SSH_STATUS_CHARS];

    (void)pszPortName;
    (void)dwMediaHdl;

    pstSsh = SshFromPrivate(pvPrivate);
    if (pstSsh == 0)
        return COM_NOT_ENOUGH_MEMORY;

    SshStopProcess(pstSsh);
    pstSsh->cbRxHead = 0;
    pstSsh->cbRxTail = 0;
    pstSsh->cbRxUsed = 0;
    pstSsh->cbIbm3270Record = 0;
    pstSsh->fIbm3270Started = FALSE;
    pstSsh->fIbm3270E = FALSE;
    pstSsh->nvtState = NVT_THRU;
    pstSsh->nSbLen = 0;
    pstSsh->fSbIac = FALSE;
    pstSsh->achError[0] = L'\0';

    ZeroMemory(&stOptions, sizeof(stOptions));
    stOptions.nMode = (pstSsh->nMode == CNCT_TRANSPORT_SSH_INTERACTIVE) ?
            UTSSH_MODE_INTERACTIVE : UTSSH_MODE_TUNNEL;
    stOptions.iSshPort = pstSsh->iSshPort > 0 ? pstSsh->iSshPort : 22;
    stOptions.iTargetPort = pstSsh->iTargetPort > 0 ? pstSsh->iTargetPort : 23;
    stOptions.fAgent = pstSsh->fAgent;
    stOptions.iRows = 24;
    stOptions.iCols = 80;
    SshCopy(stOptions.achTerminalType,
            sizeof(stOptions.achTerminalType) / sizeof(WCHAR), L"xterm");
    if (pstSsh->achSshHost[0] != L'\0')
        SshCopy(stOptions.achSshHost,
                sizeof(stOptions.achSshHost) / sizeof(WCHAR),
                pstSsh->achSshHost);
    else if (pstSsh->achSshTargetHost[0] != L'\0')
        SshCopy(stOptions.achSshHost,
                sizeof(stOptions.achSshHost) / sizeof(WCHAR),
                pstSsh->achSshTargetHost);
    else
        {
        SshSetError(pstSsh, L"SSH host is not configured.");
        return COM_DEVICE_INVALID_SETTING;
        }

    SshCopy(stOptions.achSshUser,
            sizeof(stOptions.achSshUser) / sizeof(WCHAR), pstSsh->achSshUser);
    SshCopy(stOptions.achConfigPath,
            sizeof(stOptions.achConfigPath) / sizeof(WCHAR),
            pstSsh->achSshConfigPath);
    SshCopy(stOptions.achKnownHostsPath,
            sizeof(stOptions.achKnownHostsPath) / sizeof(WCHAR),
            pstSsh->achSshKnownHostsPath);
    SshCopy(stOptions.achIdentityFiles,
            sizeof(stOptions.achIdentityFiles) / sizeof(WCHAR),
            pstSsh->achSshIdentityFiles);
    SshCopy(stOptions.achTargetHost,
            sizeof(stOptions.achTargetHost) / sizeof(WCHAR),
            pstSsh->achSshTargetHost);
    SshCopy(stOptions.achProxy,
            sizeof(stOptions.achProxy) / sizeof(WCHAR), pstSsh->achSshProxy);
    SshCopy(stOptions.achForwardings,
            sizeof(stOptions.achForwardings) / sizeof(WCHAR),
            pstSsh->achSshForwardings);

    if (stOptions.nMode == UTSSH_MODE_TUNNEL &&
            stOptions.achTargetHost[0] == L'\0')
        {
        SshSetError(pstSsh, L"SSH tunnel target host is not configured.");
        return COM_DEVICE_INVALID_SETTING;
        }

    pstSsh->pstSession = utssh_create();
    if (pstSsh->pstSession == 0)
        return COM_NOT_ENOUGH_MEMORY;
    utssh_set_options(pstSsh->pstSession, &stOptions);

    if (!utssh_connect(pstSsh->pstSession))
        {
        utssh_query_status(pstSsh->pstSession, achStatus,
                sizeof(achStatus) / sizeof(WCHAR));
        SshSetError(pstSsh, achStatus);
        SshStopProcess(pstSsh);
        return COM_DEVICE_ERROR;
        }

    pstSsh->fActivated = TRUE;
    pstSsh->fConnected = utssh_is_connected(pstSsh->pstSession);
    SshSetConnectedStatus(pstSsh);

    if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL &&
            SshGetEmulatorId(pstSsh) == EMU_IBM3270)
        SshMaybeStartIbm3270(pstSsh);

    ComNotify(pstSsh->hCom, CONNECT);
    return COM_OK;
    }

static int WINAPI SshPortDeactivate(void *pvPrivate)
    {
    ST_SSHCOM *pstSsh;

    pstSsh = SshFromPrivate(pvPrivate);
    if (pstSsh == 0)
        return COM_OK;

    SshStopProcess(pstSsh);
    pstSsh->fActivated = FALSE;
    pstSsh->fConnected = FALSE;
    SshSetStatus(pstSsh, L"SSH disconnected");
    return COM_OK;
    }

static int WINAPI SshPortConnected(void *pvPrivate)
    {
    ST_SSHCOM *pstSsh;

    pstSsh = SshFromPrivate(pvPrivate);
    if (pstSsh == 0 || pstSsh->pstSession == 0 || !pstSsh->fConnected)
        return FALSE;

    if (!utssh_is_connected(pstSsh->pstSession))
        {
        pstSsh->fConnected = FALSE;
        return FALSE;
        }

    return TRUE;
    }

static int WINAPI SshRcvRefill(void *pvPrivate)
    {
    ST_SSHCOM *pstSsh;
    ST_COM_CONTROL *pstComControl;
    BYTE abFiltered[SSH_READ_CHUNK];
    int cbRead;
    int cbFiltered;
    int nEmu;

    pstSsh = SshFromPrivate(pvPrivate);
    if (pstSsh == 0)
        return FALSE;

    cbRead = SshPopBytes(pstSsh, pstSsh->abRead, sizeof(pstSsh->abRead));
    if (cbRead <= 0 && pstSsh->pstSession != 0)
        cbRead = utssh_receive(pstSsh->pstSession, pstSsh->abRead,
                sizeof(pstSsh->abRead));
    if (cbRead <= 0)
        {
        ComNotify(pstSsh->hCom, NODATA);
        return FALSE;
        }

    nEmu = SshGetEmulatorId(pstSsh);

#if defined(INCL_VIDEOTEX)
    if (nEmu == EMU_VIDEOTEX)
        {
        emuVideotexBytesIn(SshGetEmu(pstSsh), pstSsh->abRead, cbRead);
        return FALSE;
        }
#endif

#if defined(INCL_IBM5250)
    if (nEmu == EMU_IBM5250)
        {
        emuIbm5250BytesIn(SshGetEmu(pstSsh), pstSsh->abRead, cbRead);
        return FALSE;
        }
#endif

    if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL && nEmu == EMU_IBM3270)
        {
        SshMaybeStartIbm3270(pstSsh);
        SshProcessTelnetBytes(pstSsh, pstSsh->abRead, cbRead,
                abFiltered, sizeof(abFiltered), TRUE);
        return FALSE;
        }

    if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL)
        {
        cbFiltered = SshProcessTelnetBytes(pstSsh, pstSsh->abRead, cbRead,
                abFiltered, sizeof(abFiltered), FALSE);
        if (cbFiltered <= 0)
            return FALSE;
        memcpy(pstSsh->abRead, abFiltered, cbFiltered);
        cbRead = cbFiltered;
        }

    pstComControl = (ST_COM_CONTROL *)pstSsh->hCom;
    pstComControl->puchRBData = pstSsh->abRead;
    pstComControl->puchRBDataLimit = pstSsh->abRead + cbRead;
    return TRUE;
    }

static int WINAPI SshRcvClear(void *pvPrivate)
    {
    ST_SSHCOM *pstSsh;

    pstSsh = SshFromPrivate(pvPrivate);
    if (pstSsh == 0)
        return COM_OK;

    EnterCriticalSection(&pstSsh->cs);
    pstSsh->cbRxHead = 0;
    pstSsh->cbRxTail = 0;
    pstSsh->cbRxUsed = 0;
    LeaveCriticalSection(&pstSsh->cs);
    return COM_OK;
    }

static int WINAPI SshSndBufrSend(void *pvPrivate, void *pvBufr, int nCount)
    {
    ST_SSHCOM *pstSsh;
    BYTE *pbSrc;
    BYTE *pbSend;
    BYTE *pbHeap;
    int nEmu;
    int i;
    int j;
    int fEscape;
    int iResult;

    pstSsh = SshFromPrivate(pvPrivate);
    if (pstSsh == 0 || pvBufr == 0 || nCount <= 0)
        return COM_FAILED;

    if (!SshPortConnected(pvPrivate))
        return COM_PORT_NOT_OPEN;

    pbSrc = (BYTE *)pvBufr;
    pbSend = pbSrc;
    pbHeap = 0;
    fEscape = FALSE;
    nEmu = SshGetEmulatorId(pstSsh);

    if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL &&
            nEmu != EMU_IBM3270 && nEmu != EMU_IBM5250 && nEmu != EMU_VIDEOTEX)
        fEscape = TRUE;

    if (fEscape)
        {
        pbHeap = (BYTE *)HeapAlloc(GetProcessHeap(), 0, nCount * 2);
        if (pbHeap == 0)
            return COM_NOT_ENOUGH_MEMORY;
        for (i = 0, j = 0; i < nCount; ++i)
            {
            pbHeap[j++] = pbSrc[i];
            if (pbSrc[i] == (BYTE)IAC)
                pbHeap[j++] = (BYTE)IAC;
            }
        pbSend = pbHeap;
        nCount = j;
        }

    pstSsh->fSending = TRUE;
    ComNotify(pstSsh->hCom, SEND_STARTED);
    iResult = SshSendInternal(pstSsh, pbSend, nCount);
    pstSsh->fSending = FALSE;
    ComNotify(pstSsh->hCom, SEND_DONE);

    if (pbHeap != 0)
        HeapFree(GetProcessHeap(), 0, pbHeap);

    return iResult ? COM_OK : COM_FAILED;
    }

static int WINAPI SshSndBufrIsBusy(void *pvPrivate)
    {
    ST_SSHCOM *pstSsh;

    pstSsh = SshFromPrivate(pvPrivate);
    return (pstSsh != 0 && pstSsh->fSending) ? TRUE : FALSE;
    }

static int WINAPI SshSndBufrClear(void *pvPrivate)
    {
    (void)pvPrivate;
    return COM_OK;
    }

static int WINAPI SshSndBufrQuery(void *pvPrivate, unsigned *pafStatus,
        long *plHandshakeDelay)
    {
    (void)pvPrivate;
    if (pafStatus)
        *pafStatus = 0;
    if (plHandshakeDelay)
        *plHandshakeDelay = 0;
    return COM_OK;
    }

static int WINAPI SshDeviceSpecial(void *pvPrivate,
        const WCHAR *pszInstructions, WCHAR *pszResults, int nBufrSize)
    {
    ST_SSHCOM *pstSsh;
    const WCHAR *pszValue;
    BYTE abCmd[3];

    pstSsh = SshFromPrivate(pvPrivate);
    if (pszResults && nBufrSize > 0)
        pszResults[0] = L'\0';
    if (pstSsh == 0 || pszInstructions == 0)
        return COM_FAILED;

    if (SshStartsWith(pszInstructions, L"QUERY ISCONNECTED"))
        {
        if (pszResults && nBufrSize > 0)
            wsprintf(pszResults, L"%d", SshPortConnected(pvPrivate));
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"QUERY STATUS"))
        {
        if (pszResults && nBufrSize > 0)
            lstrcpyn(pszResults, pstSsh->achStatus, nBufrSize);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"QUERY ERROR"))
        {
        if (pszResults && nBufrSize > 0)
            lstrcpyn(pszResults, pstSsh->achError, nBufrSize);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"Send Break"))
        {
        abCmd[0] = (BYTE)IAC;
        abCmd[1] = (BYTE)BREAK;
        if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL)
            SshSendInternal(pstSsh, abCmd, 2);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"Send IP"))
        {
        abCmd[0] = (BYTE)IAC;
        abCmd[1] = (BYTE)IP;
        if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL)
            SshSendInternal(pstSsh, abCmd, 2);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"Update Terminal Size"))
        {
        if (pstSsh->nMode == CNCT_TRANSPORT_SSH_TUNNEL)
            SshSendNAWS(pstSsh);
        return COM_OK;
        }

    if (!SshStartsWith(pszInstructions, L"SET "))
        return COM_NOT_SUPPORTED;

    pszValue = wcschr(pszInstructions, L'=');
    if (pszValue == 0)
        return COM_DEVICE_INVALID_SETTING;
    ++pszValue;

    if (SshStartsWith(pszInstructions, L"SET MODE="))
        {
        if (SshStartsWith(pszValue, L"SSH Interactive") ||
                SshStartsWith(pszValue, L"INTERACTIVE"))
            pstSsh->nMode = CNCT_TRANSPORT_SSH_INTERACTIVE;
        else if (SshStartsWith(pszValue, L"SSH Tunnel") ||
                SshStartsWith(pszValue, L"TUNNEL"))
            pstSsh->nMode = CNCT_TRANSPORT_SSH_TUNNEL;
        else
            pstSsh->nMode = SshSetInt(pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET SSHHOST="))
        {
        SshCopy(pstSsh->achSshHost,
                sizeof(pstSsh->achSshHost) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET SSHPORT="))
        {
        pstSsh->iSshPort = SshSetInt(pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET SSHUSER="))
        {
        SshCopy(pstSsh->achSshUser,
                sizeof(pstSsh->achSshUser) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET SSHCONFIG="))
        {
        SshCopy(pstSsh->achSshConfigPath,
                sizeof(pstSsh->achSshConfigPath) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET KNOWNHOSTS="))
        {
        SshCopy(pstSsh->achSshKnownHostsPath,
                sizeof(pstSsh->achSshKnownHostsPath) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET IDENTITIES="))
        {
        SshCopy(pstSsh->achSshIdentityFiles,
                sizeof(pstSsh->achSshIdentityFiles) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET TARGETHOST="))
        {
        SshCopy(pstSsh->achSshTargetHost,
                sizeof(pstSsh->achSshTargetHost) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET TARGETPORT="))
        {
        pstSsh->iTargetPort = SshSetInt(pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET PROXY="))
        {
        SshCopy(pstSsh->achSshProxy,
                sizeof(pstSsh->achSshProxy) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET FORWARDINGS="))
        {
        SshCopy(pstSsh->achSshForwardings,
                sizeof(pstSsh->achSshForwardings) / sizeof(WCHAR), pszValue);
        return COM_OK;
        }

    if (SshStartsWith(pszInstructions, L"SET AGENT="))
        {
        pstSsh->fAgent = SshSetInt(pszValue) ? TRUE : FALSE;
        return COM_OK;
        }

    return COM_NOT_SUPPORTED;
    }

static int WINAPI SshPortConfigure(void *pvPrivate)
    {
    (void)pvPrivate;
    return COM_OK;
    }

static void SshStopProcess(ST_SSHCOM *pstSsh)
    {
    if (pstSsh == 0)
        return;

    if (pstSsh->pstSession != 0)
        {
        utssh_disconnect(pstSsh->pstSession);
        utssh_destroy(pstSsh->pstSession);
        pstSsh->pstSession = 0;
        }

    pstSsh->fConnected = FALSE;
    pstSsh->fSending = FALSE;
    }

static int SshQueueBytes(ST_SSHCOM *pstSsh, const BYTE *pbData, int cbData)
    {
    int i;

    if (pstSsh == 0 || pbData == 0 || cbData <= 0)
        return FALSE;

    EnterCriticalSection(&pstSsh->cs);
    for (i = 0; i < cbData; ++i)
        {
        if (pstSsh->cbRxUsed == pstSsh->cbRxSize)
            {
            pstSsh->cbRxHead = (pstSsh->cbRxHead + 1) % pstSsh->cbRxSize;
            --pstSsh->cbRxUsed;
            }
        pstSsh->pbRx[pstSsh->cbRxTail] = pbData[i];
        pstSsh->cbRxTail = (pstSsh->cbRxTail + 1) % pstSsh->cbRxSize;
        ++pstSsh->cbRxUsed;
        }
    LeaveCriticalSection(&pstSsh->cs);
    return TRUE;
    }

static int SshPopBytes(ST_SSHCOM *pstSsh, BYTE *pbData, int cbData)
    {
    int cbOut;

    if (pstSsh == 0 || pbData == 0 || cbData <= 0)
        return 0;

    cbOut = 0;
    EnterCriticalSection(&pstSsh->cs);
    while (cbOut < cbData && pstSsh->cbRxUsed > 0)
        {
        pbData[cbOut++] = pstSsh->pbRx[pstSsh->cbRxHead];
        pstSsh->cbRxHead = (pstSsh->cbRxHead + 1) % pstSsh->cbRxSize;
        --pstSsh->cbRxUsed;
        }
    LeaveCriticalSection(&pstSsh->cs);
    return cbOut;
    }

static int SshSendInternal(ST_SSHCOM *pstSsh, const BYTE *pbData, int cbData)
    {
    int cbWritten;

    if (pstSsh == 0 || pstSsh->pstSession == 0 || pbData == 0 || cbData <= 0)
        return FALSE;

    cbWritten = utssh_send(pstSsh->pstSession, pbData, cbData);
    if (cbWritten != cbData)
        {
        SshSetError(pstSsh, L"OpenSSH channel write failed.");
        pstSsh->fConnected = FALSE;
        return FALSE;
        }

    return TRUE;
    }

static HEMU SshGetEmu(ST_SSHCOM *pstSsh)
    {
    HSESSION hSession;

    if (pstSsh == 0 || pstSsh->hCom == 0)
        return 0;

    if (ComGetSession(pstSsh->hCom, &hSession) != COM_OK || hSession == 0)
        return 0;

    return sessQueryEmuHdl(hSession);
    }

static int SshGetEmulatorId(ST_SSHCOM *pstSsh)
    {
    HEMU hEmu;

    hEmu = SshGetEmu(pstSsh);
    return hEmu ? emuQueryEmulatorId(hEmu) : 0;
    }

static void SshMaybeStartIbm3270(ST_SSHCOM *pstSsh)
    {
    if (pstSsh == 0 || pstSsh->fIbm3270Started)
        return;

    pstSsh->fIbm3270Started = TRUE;
    pstSsh->nvtState = NVT_THRU;
    pstSsh->cbIbm3270Record = 0;
    SshSendTelnetCommand(pstSsh, DO, TELOPT_TN3270E);
    SshSendTelnetCommand(pstSsh, WILL, TELOPT_TN3270E);
    SshSendTelnetCommand(pstSsh, WILL, TELOPT_TTYPE);
    SshSendTelnetCommand(pstSsh, DO, TELOPT_BINARY);
    SshSendTelnetCommand(pstSsh, WILL, TELOPT_BINARY);
    SshSendTelnetCommand(pstSsh, DO, TELOPT_EOR);
    SshSendTelnetCommand(pstSsh, WILL, TELOPT_EOR);
    }

static void SshSendTelnetCommand(ST_SSHCOM *pstSsh, int nCommand, int nOption)
    {
    BYTE ab[3];

    ab[0] = (BYTE)IAC;
    ab[1] = (BYTE)nCommand;
    ab[2] = (BYTE)nOption;
    SshSendInternal(pstSsh, ab, sizeof(ab));
    }

static int SshProcessTelnetBytes(ST_SSHCOM *pstSsh, const BYTE *pbIn,
        int cbIn, BYTE *pbOut, int cbOut, int fIbm3270)
    {
    int i;
    int cbOutUsed;
    BYTE b;

    cbOutUsed = 0;

    for (i = 0; i < cbIn; ++i)
        {
        b = pbIn[i];

        switch (pstSsh->nvtState)
            {
        case NVT_THRU:
            if (b == (BYTE)IAC)
                {
                pstSsh->nvtState = NVT_IAC;
                break;
                }
            if (fIbm3270)
                {
                if (pstSsh->cbIbm3270Record <
                        (int)sizeof(pstSsh->abIbm3270Record))
                    pstSsh->abIbm3270Record[pstSsh->cbIbm3270Record++] = b;
                }
            else if (cbOutUsed < cbOut)
                pbOut[cbOutUsed++] = b;
            break;

        case NVT_IAC:
            switch (b)
                {
            case IAC:
                if (fIbm3270)
                    {
                    if (pstSsh->cbIbm3270Record <
                            (int)sizeof(pstSsh->abIbm3270Record))
                        pstSsh->abIbm3270Record[pstSsh->cbIbm3270Record++] = b;
                    }
                else if (cbOutUsed < cbOut)
                    pbOut[cbOutUsed++] = b;
                pstSsh->nvtState = NVT_THRU;
                break;

            case DO:
                pstSsh->nvtState = NVT_DO;
                break;

            case DONT:
                pstSsh->nvtState = NVT_DONT;
                break;

            case WILL:
                pstSsh->nvtState = NVT_WILL;
                break;

            case WONT:
                pstSsh->nvtState = NVT_WONT;
                break;

            case SB:
                pstSsh->nvtState = NVT_SB;
                pstSsh->nSbLen = 0;
                pstSsh->fSbIac = FALSE;
                break;

            case EOR:
                if (fIbm3270)
                    SshDeliverIbm3270Record(pstSsh);
                pstSsh->nvtState = NVT_THRU;
                break;

            default:
                pstSsh->nvtState = NVT_THRU;
                break;
                }
            break;

        case NVT_DO:
            if (SshOptionSupportedLocal(b, fIbm3270))
                {
                SshSendTelnetCommand(pstSsh, WILL, b);
                if (b == TELOPT_NAWS)
                    SshSendNAWS(pstSsh);
                }
            else
                SshSendTelnetCommand(pstSsh, WONT, b);
            pstSsh->nvtState = NVT_THRU;
            break;

        case NVT_DONT:
            SshSendTelnetCommand(pstSsh, WONT, b);
            pstSsh->nvtState = NVT_THRU;
            break;

        case NVT_WILL:
            if (SshOptionSupportedRemote(b, fIbm3270))
                SshSendTelnetCommand(pstSsh, DO, b);
            else
                SshSendTelnetCommand(pstSsh, DONT, b);
            pstSsh->nvtState = NVT_THRU;
            break;

        case NVT_WONT:
            SshSendTelnetCommand(pstSsh, DONT, b);
            if (b == TELOPT_TN3270E)
                {
                pstSsh->fIbm3270E = FALSE;
                emuIbm3270SetTelnetMode(SshGetEmu(pstSsh), FALSE);
                }
            pstSsh->nvtState = NVT_THRU;
            break;

        case NVT_SB:
            pstSsh->nSbOption = b;
            pstSsh->nSbLen = 0;
            pstSsh->fSbIac = FALSE;
            if (b == TELOPT_TTYPE)
                pstSsh->nvtState = NVT_SB_TT;
            else if (b == TELOPT_TN3270E)
                pstSsh->nvtState = NVT_SB_TN3270E;
            else
                pstSsh->nvtState = NVT_SB_TN3270E;
            break;

        case NVT_SB_TT:
        case NVT_SB_TT_S:
        case NVT_SB_TN3270E:
            if (pstSsh->fSbIac)
                {
                pstSsh->fSbIac = FALSE;
                if (b == (BYTE)SE)
                    {
                    if (pstSsh->nSbOption == TELOPT_TTYPE &&
                            pstSsh->nSbLen > 0 &&
                            pstSsh->abSbBuf[0] == TELQUAL_SEND)
                        SshSendTerminalType(pstSsh);
                    else if (pstSsh->nSbOption == TELOPT_TN3270E)
                        SshProcessIbm3270Subneg(pstSsh);
                    pstSsh->nvtState = NVT_THRU;
                    }
                else if (b == (BYTE)IAC)
                    SshAppendSubnegByte(pstSsh, b);
                else
                    pstSsh->nvtState = NVT_THRU;
                }
            else if (b == (BYTE)IAC)
                pstSsh->fSbIac = TRUE;
            else
                SshAppendSubnegByte(pstSsh, b);
            break;

        default:
            pstSsh->nvtState = NVT_THRU;
            break;
            }
        }

    return cbOutUsed;
    }

static int SshOptionSupportedLocal(int nOption, int fIbm3270)
    {
    if (nOption == TELOPT_TTYPE || nOption == TELOPT_NAWS ||
            nOption == TELOPT_BINARY || nOption == TELOPT_SGA)
        return TRUE;

    if (fIbm3270 && (nOption == TELOPT_EOR || nOption == TELOPT_TN3270E))
        return TRUE;

    return FALSE;
    }

static int SshOptionSupportedRemote(int nOption, int fIbm3270)
    {
    if (nOption == TELOPT_ECHO || nOption == TELOPT_SGA ||
            nOption == TELOPT_BINARY)
        return TRUE;

    if (fIbm3270 && (nOption == TELOPT_EOR || nOption == TELOPT_TN3270E))
        return TRUE;

    return FALSE;
    }

static void SshAppendSubnegByte(ST_SSHCOM *pstSsh, BYTE b)
    {
    if (pstSsh->nSbLen < (int)sizeof(pstSsh->abSbBuf))
        pstSsh->abSbBuf[pstSsh->nSbLen++] = b;
    }

static void SshSendTerminalType(ST_SSHCOM *pstSsh)
    {
    BYTE abOut[320];
    STEMUSET stEmuSet;
    HEMU hEmu;
    char achTermType[EMU_MAX_TELNETID];
    const char *pszTerm;
    int nEmu;
    int i;
    int pos;

    memset(&stEmuSet, 0, sizeof(stEmuSet));
    hEmu = SshGetEmu(pstSsh);
    if (hEmu)
        emuQuerySettings(hEmu, &stEmuSet);

    nEmu = hEmu ? emuQueryEmulatorId(hEmu) : EMU_VT100;
    SshWideToProtocolAscii(achTermType, sizeof(achTermType),
            stEmuSet.acTelnetId);
    pszTerm = achTermType;
    if (nEmu == EMU_IBM3270 && achTermType[0] == '\0')
        pszTerm = "IBM-3278-2-E";
    else if (achTermType[0] == '\0')
        {
        switch (nEmu)
            {
        case EMU_ANSI:
            pszTerm = "ANSI";
            break;
        case EMU_TTY:
            pszTerm = "TELETYPE-33";
            break;
        case EMU_VT52:
            pszTerm = "DEC-VT52";
            break;
        case EMU_VT220:
            pszTerm = "DEC-VT220";
            break;
        case EMU_VT320:
            pszTerm = "DEC-VT320";
            break;
        case EMU_VTUTF8:
            pszTerm = "VT-UTF8";
            break;
        case EMU_IBM3270:
            pszTerm = "IBM-3278-2-E";
            break;
        default:
            pszTerm = "DEC-VT100";
            break;
            }
        }

    pos = 0;
    abOut[pos++] = (BYTE)IAC;
    abOut[pos++] = (BYTE)SB;
    abOut[pos++] = (BYTE)TELOPT_TTYPE;
    abOut[pos++] = (BYTE)TELQUAL_IS;
    for (i = 0; pszTerm[i] != '\0' && pos < (int)sizeof(abOut) - 2; ++i)
        abOut[pos++] = (BYTE)pszTerm[i];
    abOut[pos++] = (BYTE)IAC;
    abOut[pos++] = (BYTE)SE;

    SshSendInternal(pstSsh, abOut, pos);
    }

static void SshSendNAWS(ST_SSHCOM *pstSsh)
    {
    HEMU hEmu;
    int iRows;
    int iCols;
    BYTE abOut[9];

    hEmu = SshGetEmu(pstSsh);
    iRows = 24;
    iCols = 80;
    if (hEmu)
        emuQueryRowsCols(hEmu, &iRows, &iCols);

    abOut[0] = (BYTE)IAC;
    abOut[1] = (BYTE)SB;
    abOut[2] = (BYTE)TELOPT_NAWS;
    abOut[3] = (BYTE)((iCols >> 8) & 0xff);
    abOut[4] = (BYTE)(iCols & 0xff);
    abOut[5] = (BYTE)((iRows >> 8) & 0xff);
    abOut[6] = (BYTE)(iRows & 0xff);
    abOut[7] = (BYTE)IAC;
    abOut[8] = (BYTE)SE;

    SshSendInternal(pstSsh, abOut, sizeof(abOut));
    }

static void SshProcessIbm3270Subneg(ST_SSHCOM *pstSsh)
    {
    BYTE *buf;
    int len;

    buf = pstSsh->abSbBuf;
    len = pstSsh->nSbLen;

    if (len < 1)
        return;

    if (len >= 2 && buf[0] == UT_TN3270E_OP_SEND &&
            buf[1] == UT_TN3270E_OP_DEVICE_TYPE)
        {
        SshSendIbm3270DeviceType(pstSsh);
        return;
        }

    if (len >= 2 && buf[0] == UT_TN3270E_OP_DEVICE_TYPE &&
            buf[1] == UT_TN3270E_OP_IS)
        {
        pstSsh->fIbm3270E = TRUE;
        emuIbm3270SetTelnetMode(SshGetEmu(pstSsh), TRUE);
        return;
        }

    if (len >= 2 && buf[0] == UT_TN3270E_OP_FUNCTIONS &&
            buf[1] == UT_TN3270E_OP_REQUEST)
        {
        SshSendIbm3270Functions(pstSsh);
        return;
        }

    if (buf[0] == UT_TN3270E_OP_REJECT)
        {
        pstSsh->fIbm3270E = FALSE;
        emuIbm3270SetTelnetMode(SshGetEmu(pstSsh), FALSE);
        }
    }

static void SshSendIbm3270DeviceType(ST_SSHCOM *pstSsh)
    {
    BYTE abOut[256];
    STEMUSET stEmuSet;
    HEMU hEmu;
    char achTerm[EMU_IBM3270_MAX_LUNAME];
    char achTelnetId[EMU_MAX_TELNETID];
    char achLu[EMU_IBM3270_MAX_LUNAME];
    const char *pszTerm;
    const char *pszLu;
    int pos;
    int i;

    memset(&stEmuSet, 0, sizeof(stEmuSet));
    hEmu = SshGetEmu(pstSsh);
    if (hEmu)
        emuQuerySettings(hEmu, &stEmuSet);

    SshWideToProtocolAscii(achTerm, sizeof(achTerm),
            stEmuSet.acIbm3270DeviceName);
    pszTerm = achTerm;
    if (pszTerm[0] == '\0')
        {
        SshWideToProtocolAscii(achTelnetId, sizeof(achTelnetId),
                stEmuSet.acTelnetId);
        pszTerm = achTelnetId[0] ? achTelnetId : "IBM-3278-2-E";
        }
    SshWideToProtocolAscii(achLu, sizeof(achLu),
            stEmuSet.acIbm3270LuName);
    pszLu = achLu;

    pos = 0;
    abOut[pos++] = (BYTE)IAC;
    abOut[pos++] = (BYTE)SB;
    abOut[pos++] = (BYTE)TELOPT_TN3270E;
    abOut[pos++] = (BYTE)UT_TN3270E_OP_DEVICE_TYPE;
    abOut[pos++] = (BYTE)UT_TN3270E_OP_REQUEST;
    for (i = 0; pszTerm[i] != '\0' && pos < (int)sizeof(abOut) - 8; ++i)
        abOut[pos++] = (BYTE)pszTerm[i];
    if (pszLu[0] != '\0' && pos < (int)sizeof(abOut) - 8)
        {
        abOut[pos++] = (BYTE)UT_TN3270E_OP_CONNECT;
        for (i = 0; pszLu[i] != '\0' && pos < (int)sizeof(abOut) - 3; ++i)
            abOut[pos++] = (BYTE)pszLu[i];
        }
    abOut[pos++] = (BYTE)IAC;
    abOut[pos++] = (BYTE)SE;

    SshSendInternal(pstSsh, abOut, pos);
    }

static void SshSendIbm3270Functions(ST_SSHCOM *pstSsh)
    {
    BYTE abOut[18];
    int pos;

    pos = 0;
    abOut[pos++] = (BYTE)IAC;
    abOut[pos++] = (BYTE)SB;
    abOut[pos++] = (BYTE)TELOPT_TN3270E;
    abOut[pos++] = (BYTE)UT_TN3270E_OP_FUNCTIONS;
    abOut[pos++] = (BYTE)UT_TN3270E_OP_IS;
    abOut[pos++] = (BYTE)UT_TN3270E_FUNC_BIND_IMAGE;
    abOut[pos++] = (BYTE)UT_TN3270E_FUNC_DATA_STREAM_CTL;
    abOut[pos++] = (BYTE)UT_TN3270E_FUNC_RESPONSES;
    abOut[pos++] = (BYTE)UT_TN3270E_FUNC_SCS_CTL_CODES;
    abOut[pos++] = (BYTE)UT_TN3270E_FUNC_SYSREQ;
    abOut[pos++] = (BYTE)UT_TN3270E_FUNC_CONTENTION_RESOLUTION;
    abOut[pos++] = (BYTE)IAC;
    abOut[pos++] = (BYTE)SE;

    SshSendInternal(pstSsh, abOut, pos);
    }

static void SshDeliverIbm3270Record(ST_SSHCOM *pstSsh)
    {
    HEMU hEmu;

    if (pstSsh->cbIbm3270Record <= 0)
        return;

    hEmu = SshGetEmu(pstSsh);
    if (hEmu)
        emuIbm3270RecordIn(hEmu, pstSsh->abIbm3270Record,
                pstSsh->cbIbm3270Record);
    pstSsh->cbIbm3270Record = 0;
    }

static void SshWideToProtocolAscii(char *pszDest, int cchDest,
        const WCHAR *pszSrc)
    {
    int i;
    int j;
    WCHAR ch;

    if (pszDest == 0 || cchDest <= 0)
        return;

    pszDest[0] = '\0';
    if (pszSrc == 0)
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

static void SshCopy(WCHAR *pszDest, int cchDest, const WCHAR *pszSrc)
    {
    if (pszDest == 0 || cchDest <= 0)
        return;

    if (pszSrc == 0)
        pszSrc = L"";

    lstrcpyn(pszDest, pszSrc, cchDest);
    pszDest[cchDest - 1] = L'\0';
    }

static const WCHAR *SshSkipSpaces(const WCHAR *psz)
    {
    while (psz != 0 && (*psz == L' ' || *psz == L'\t'))
        ++psz;
    return psz;
    }

static int SshStartsWith(const WCHAR *psz, const WCHAR *pszPrefix)
    {
    int cch;

    if (psz == 0 || pszPrefix == 0)
        return FALSE;

    psz = SshSkipSpaces(psz);
    cch = lstrlenW(pszPrefix);
    return CompareStringOrdinal(psz, cch, pszPrefix, cch, TRUE) == CSTR_EQUAL;
    }

static int SshSetInt(const WCHAR *psz)
    {
    if (psz == 0)
        return 0;
    return (int)wcstol(SshSkipSpaces(psz), NULL, 10);
    }

#else

int ComLoadSshDriver(HCOM pstCom)
    {
    (void)pstCom;
    return COM_NOT_SUPPORTED;
    }

#endif
