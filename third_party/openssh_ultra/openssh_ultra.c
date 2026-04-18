#include "includes.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincred.h>

#include "openssh_ultra.h"

#include "ssh_api.h"
#include "ssh2.h"
#include "sshbuf.h"
#include "ssherr.h"
#include "digest.h"
#include "sshkey.h"
#include "authfile.h"
#include "hostfile.h"
#include "packet.h"
#include "log.h"
#include "misc.h"
#include "readconf.h"

#define UTSSH_RX_QUEUE_SIZE     131072
#define UTSSH_NET_CHUNK         8192
#define UTSSH_CONNECT_TIMEOUT   30000
#define UTSSH_CHANNEL_WINDOW    (2 * 1024 * 1024)
#define UTSSH_CHANNEL_PACKET    32768

struct utssh_session
    {
    UTSSH_OPTIONS stOptions;
    SOCKET s;
    int fWinsockStarted;
    int fConnected;
    int fAuthenticated;
    int fChannelOpen;
    struct ssh *ssh;
    uint32_t uLocalChannel;
    uint32_t uRemoteChannel;
    uint32_t uRemoteWindow;
    uint32_t uRemoteMaxPacket;
    BYTE *pbRx;
    int cbRxHead;
    int cbRxTail;
    int cbRxUsed;
    WCHAR achStatus[UTSSH_STATUS_CHARS];
    };

static void utssh_copy(WCHAR *pszDest, int cchDest, const WCHAR *pszSrc)
    {
    if (pszDest == 0 || cchDest <= 0)
        return;

    if (pszSrc == 0)
        pszSrc = L"";

    lstrcpyn(pszDest, pszSrc, cchDest);
    pszDest[cchDest - 1] = L'\0';
    }

static void utssh_set_status(UTSSH_SESSION *pstSession, const WCHAR *pszStatus)
    {
    if (pstSession == 0)
        return;

    utssh_copy(pstSession->achStatus,
            sizeof(pstSession->achStatus) / sizeof(WCHAR), pszStatus);
    }

static void utssh_set_status_a(UTSSH_SESSION *pstSession, const char *pszStatus)
    {
#if defined(UNICODE)
    WCHAR awStatus[UTSSH_STATUS_CHARS];
    MultiByteToWideChar(CP_ACP, 0, pszStatus ? pszStatus : "", -1,
            awStatus, UTSSH_STATUS_CHARS);
    utssh_set_status(pstSession, awStatus);
#else
    utssh_set_status(pstSession, pszStatus ? pszStatus : "");
#endif
    }

static void utssh_wide_to_char(const WCHAR *pszIn, char *pszOut, int cbOut)
    {
    if (pszOut == 0 || cbOut <= 0)
        return;
    pszOut[0] = '\0';
    if (pszIn == 0)
        return;
    WideCharToMultiByte(CP_ACP, 0, pszIn, -1, pszOut, cbOut, 0, 0);
    pszOut[cbOut - 1] = '\0';
    }

static void utssh_char_to_wide(const char *pszIn, WCHAR *pszOut, int cchOut)
    {
    if (pszOut == 0 || cchOut <= 0)
        return;
    pszOut[0] = L'\0';
    if (pszIn == 0)
        return;
    MultiByteToWideChar(CP_ACP, 0, pszIn, -1, pszOut, cchOut);
    pszOut[cchOut - 1] = L'\0';
    }

static void utssh_default_user(char *pszUser, int cbUser)
    {
    DWORD cchUser;

    if (pszUser == 0 || cbUser <= 0)
        return;
    pszUser[0] = '\0';
    cchUser = (DWORD)cbUser;
    if (!GetUserNameA(pszUser, &cchUser) || pszUser[0] == '\0')
        lstrcpynA(pszUser, "user", cbUser);
    }

static void utssh_expand_user_path(const char *pszPath, char *pszOut, int cbOut)
    {
    char achHome[MAX_PATH];
    DWORD cchHome;
    int cchUsed;

    if (pszOut == 0 || cbOut <= 0)
        return;

    pszOut[0] = '\0';
    if (pszPath == 0 || pszPath[0] == '\0')
        return;

    if (pszPath[0] == '~' && (pszPath[1] == '/' || pszPath[1] == '\\'))
        {
        cchHome = GetEnvironmentVariableA("USERPROFILE", achHome,
                sizeof(achHome));
        if (cchHome != 0 && cchHome < sizeof(achHome))
            {
            lstrcpynA(pszOut, achHome, cbOut);
            cchUsed = lstrlenA(pszOut);
            if (cchUsed < cbOut - 1)
                {
                pszOut[cchUsed++] = '/';
                pszOut[cchUsed] = '\0';
                lstrcpynA(pszOut + cchUsed, pszPath + 2, cbOut - cchUsed);
                }
            }
        else
            lstrcpynA(pszOut, pszPath + 2, cbOut);
        }
    else
        {
        lstrcpynA(pszOut, pszPath, cbOut);
        }

    pszOut[cbOut - 1] = '\0';
    }

static void utssh_known_hosts_path(UTSSH_SESSION *pstSession, char *pszPath,
        int cbPath)
    {
    char achConfigured[MAX_PATH];
    char achHome[MAX_PATH];
    DWORD cchHome;

    if (pszPath == 0 || cbPath <= 0)
        return;

    pszPath[0] = '\0';
    utssh_wide_to_char(pstSession->stOptions.achKnownHostsPath, achConfigured,
            sizeof(achConfigured));
    if (achConfigured[0] != '\0')
        {
        utssh_expand_user_path(achConfigured, pszPath, cbPath);
        return;
        }

    cchHome = GetEnvironmentVariableA("USERPROFILE", achHome, sizeof(achHome));
    if (cchHome != 0 && cchHome < sizeof(achHome))
        wsprintfA(pszPath, "%s/.ssh/known_hosts", achHome);
    else
        lstrcpynA(pszPath, ".ssh/known_hosts", cbPath);
    pszPath[cbPath - 1] = '\0';
    }

static void utssh_known_host_name(UTSSH_SESSION *pstSession, char *pszHost,
        int cbHost)
    {
    char achRaw[256];

    if (pszHost == 0 || cbHost <= 0)
        return;

    utssh_wide_to_char(pstSession->stOptions.achSshHost, achRaw,
            sizeof(achRaw));
    if (pstSession->stOptions.iSshPort > 0 &&
            pstSession->stOptions.iSshPort != 22)
        wsprintfA(pszHost, "[%s]:%d", achRaw, pstSession->stOptions.iSshPort);
    else
        lstrcpynA(pszHost, achRaw, cbHost);
    pszHost[cbHost - 1] = '\0';
    }

static void utssh_append_identity(UTSSH_OPTIONS *pstOptions,
        const char *pszIdentity)
    {
    WCHAR achIdentity[MAX_PATH];
    int cchUsed;
    int cchMax;

    if (pstOptions == 0 || pszIdentity == 0 || pszIdentity[0] == '\0')
        return;

    utssh_char_to_wide(pszIdentity, achIdentity,
            sizeof(achIdentity) / sizeof(WCHAR));
    cchMax = sizeof(pstOptions->achIdentityFiles) / sizeof(WCHAR);
    cchUsed = lstrlen(pstOptions->achIdentityFiles);
    if (cchUsed != 0 && cchUsed < cchMax - 1)
        {
        pstOptions->achIdentityFiles[cchUsed++] = L';';
        pstOptions->achIdentityFiles[cchUsed] = L'\0';
        }
    if (cchUsed < cchMax - 1)
        lstrcpyn(pstOptions->achIdentityFiles + cchUsed, achIdentity,
                cchMax - cchUsed);
    }

static void utssh_apply_readconf(UTSSH_SESSION *pstSession)
    {
    Options stReadOptions;
    struct passwd *pstPw;
    char achConfig[MAX_PATH];
    char achConfigured[MAX_PATH];
    char achHost[256];
    int fWantFinal;
    int i;

    if (pstSession == 0)
        return;

    utssh_wide_to_char(pstSession->stOptions.achSshHost, achHost,
            sizeof(achHost));
    if (achHost[0] == '\0')
        return;

    utssh_wide_to_char(pstSession->stOptions.achConfigPath, achConfigured,
            sizeof(achConfigured));
    if (achConfigured[0] != '\0')
        {
        utssh_expand_user_path(achConfigured, achConfig, sizeof(achConfig));
        }
    else
        {
        utssh_expand_user_path("~/.ssh/config", achConfig, sizeof(achConfig));
        }
    if (achConfig[0] == '\0')
        return;

    pstPw = getpwuid(getuid());
    initialize_options(&stReadOptions);
    fWantFinal = 0;
    if (read_config_file(achConfig, pstPw, achHost, achHost, 0,
            &stReadOptions, SSHCONF_USERCONF, &fWantFinal) == 0)
        {
        fill_default_options(&stReadOptions);
        if (pstSession->stOptions.achSshUser[0] == L'\0' &&
                stReadOptions.user != 0)
            utssh_char_to_wide(stReadOptions.user,
                    pstSession->stOptions.achSshUser,
                    sizeof(pstSession->stOptions.achSshUser) / sizeof(WCHAR));
        if (stReadOptions.hostname != 0 && stReadOptions.hostname[0] != '\0')
            utssh_char_to_wide(stReadOptions.hostname,
                    pstSession->stOptions.achSshHost,
                    sizeof(pstSession->stOptions.achSshHost) / sizeof(WCHAR));
        if (pstSession->stOptions.iSshPort == 22 && stReadOptions.port > 0)
            pstSession->stOptions.iSshPort = stReadOptions.port;
        if (pstSession->stOptions.achKnownHostsPath[0] == L'\0' &&
                stReadOptions.num_user_hostfiles > 0 &&
                stReadOptions.user_hostfiles[0] != 0)
            utssh_char_to_wide(stReadOptions.user_hostfiles[0],
                    pstSession->stOptions.achKnownHostsPath,
                    sizeof(pstSession->stOptions.achKnownHostsPath) /
                    sizeof(WCHAR));
        if (pstSession->stOptions.achIdentityFiles[0] == L'\0')
            {
            for (i = 0; i < (int)stReadOptions.num_identity_files; ++i)
                utssh_append_identity(&pstSession->stOptions,
                        stReadOptions.identity_files[i]);
            }
        if (stReadOptions.forward_agent == 1)
            pstSession->stOptions.fAgent = TRUE;
        if (stReadOptions.compression == 1)
            pstSession->stOptions.fCompression = TRUE;
        }
    free_options(&stReadOptions);
    }

static int utssh_prompt_secret(UTSSH_SESSION *pstSession, const char *pszTarget,
        const char *pszMessage, const char *pszUser, char *pszSecret,
        int cbSecret)
    {
    typedef DWORD (WINAPI *PFN_CREDUI_PROMPT_A)(PCREDUI_INFOA, PCSTR,
            PCtxtHandle, DWORD, PSTR, ULONG, PSTR, ULONG, BOOL *, DWORD);
    HMODULE hCredUi;
    PFN_CREDUI_PROMPT_A pfnPrompt;
    CREDUI_INFOA stInfo;
    char achUser[128];
    BOOL fSave;
    DWORD dwResult;

    if (pszSecret == 0 || cbSecret <= 0)
        return FALSE;
    pszSecret[0] = '\0';

    hCredUi = LoadLibraryW(L"credui.dll");
    if (hCredUi == 0)
        {
        utssh_set_status(pstSession, L"SSH credential prompt is unavailable");
        return FALSE;
        }

    pfnPrompt = (PFN_CREDUI_PROMPT_A)GetProcAddress(hCredUi,
            "CredUIPromptForCredentialsA");
    if (pfnPrompt == 0)
        {
        FreeLibrary(hCredUi);
        utssh_set_status(pstSession, L"SSH credential prompt is unavailable");
        return FALSE;
        }

    ZeroMemory(&stInfo, sizeof(stInfo));
    stInfo.cbSize = sizeof(stInfo);
    stInfo.pszCaptionText = "UltraTerminal SSH Authentication";
    stInfo.pszMessageText = pszMessage;
    achUser[0] = '\0';
    if (pszUser != 0)
        lstrcpynA(achUser, pszUser, sizeof(achUser));
    fSave = FALSE;

    dwResult = pfnPrompt(&stInfo,
            pszTarget != 0 && pszTarget[0] != '\0' ? pszTarget : "SSH",
            0, 0, achUser, sizeof(achUser), pszSecret, cbSecret, &fSave,
            CREDUI_FLAGS_GENERIC_CREDENTIALS |
            CREDUI_FLAGS_DO_NOT_PERSIST |
            CREDUI_FLAGS_ALWAYS_SHOW_UI);
    FreeLibrary(hCredUi);

    if (dwResult != NO_ERROR)
        {
        SecureZeroMemory(pszSecret, cbSecret);
        return FALSE;
        }

    return TRUE;
    }

static int utssh_queue_bytes(UTSSH_SESSION *pstSession, const BYTE *pbData,
        int cbData)
    {
    int cbStored;
    int cbFirst;

    if (pstSession == 0 || pbData == 0 || cbData <= 0)
        return 0;

    cbStored = 0;
    while (cbStored < cbData && pstSession->cbRxUsed < UTSSH_RX_QUEUE_SIZE)
        {
        cbFirst = UTSSH_RX_QUEUE_SIZE - pstSession->cbRxTail;
        if (cbFirst > cbData - cbStored)
            cbFirst = cbData - cbStored;
        if (cbFirst > UTSSH_RX_QUEUE_SIZE - pstSession->cbRxUsed)
            cbFirst = UTSSH_RX_QUEUE_SIZE - pstSession->cbRxUsed;
        CopyMemory(pstSession->pbRx + pstSession->cbRxTail,
                pbData + cbStored, cbFirst);
        pstSession->cbRxTail =
                (pstSession->cbRxTail + cbFirst) % UTSSH_RX_QUEUE_SIZE;
        pstSession->cbRxUsed += cbFirst;
        cbStored += cbFirst;
        }
    return cbStored;
    }

static int utssh_pop_bytes(UTSSH_SESSION *pstSession, BYTE *pbData, int cbData)
    {
    int cbRead;
    int cbFirst;

    if (pstSession == 0 || pbData == 0 || cbData <= 0)
        return 0;

    cbRead = 0;
    while (cbRead < cbData && pstSession->cbRxUsed > 0)
        {
        cbFirst = UTSSH_RX_QUEUE_SIZE - pstSession->cbRxHead;
        if (cbFirst > cbData - cbRead)
            cbFirst = cbData - cbRead;
        if (cbFirst > pstSession->cbRxUsed)
            cbFirst = pstSession->cbRxUsed;
        CopyMemory(pbData + cbRead, pstSession->pbRx + pstSession->cbRxHead,
                cbFirst);
        pstSession->cbRxHead =
                (pstSession->cbRxHead + cbFirst) % UTSSH_RX_QUEUE_SIZE;
        pstSession->cbRxUsed -= cbFirst;
        cbRead += cbFirst;
        }
    return cbRead;
    }

UTSSH_SESSION *utssh_create(void)
    {
    UTSSH_SESSION *pstSession;

    pstSession = (UTSSH_SESSION *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(UTSSH_SESSION));
    if (pstSession == 0)
        return 0;

    pstSession->pbRx = (BYTE *)HeapAlloc(GetProcessHeap(), 0,
            UTSSH_RX_QUEUE_SIZE);
    if (pstSession->pbRx == 0)
        {
        HeapFree(GetProcessHeap(), 0, pstSession);
        return 0;
        }

    pstSession->s = INVALID_SOCKET;
    pstSession->stOptions.iSshPort = 22;
    pstSession->stOptions.nMode = UTSSH_MODE_TUNNEL;
    pstSession->stOptions.iRows = 24;
    pstSession->stOptions.iCols = 80;
    utssh_copy(pstSession->stOptions.achTerminalType,
            sizeof(pstSession->stOptions.achTerminalType) / sizeof(WCHAR),
            L"xterm");
    utssh_set_status(pstSession, L"Embedded OpenSSH runtime idle");
    return pstSession;
    }

void utssh_destroy(UTSSH_SESSION *pstSession)
    {
    if (pstSession == 0)
        return;

    utssh_disconnect(pstSession);
    if (pstSession->fWinsockStarted)
        {
        WSACleanup();
        pstSession->fWinsockStarted = FALSE;
        }
    if (pstSession->pbRx != 0)
        HeapFree(GetProcessHeap(), 0, pstSession->pbRx);
    HeapFree(GetProcessHeap(), 0, pstSession);
    }

void utssh_set_options(UTSSH_SESSION *pstSession,
        const UTSSH_OPTIONS *pstOptions)
    {
    if (pstSession == 0 || pstOptions == 0)
        return;

    pstSession->stOptions = *pstOptions;
    if (pstSession->stOptions.iSshPort <= 0)
        pstSession->stOptions.iSshPort = 22;
    if (pstSession->stOptions.iRows <= 0)
        pstSession->stOptions.iRows = 24;
    if (pstSession->stOptions.iCols <= 0)
        pstSession->stOptions.iCols = 80;
    if (pstSession->stOptions.achTerminalType[0] == L'\0')
        utssh_copy(pstSession->stOptions.achTerminalType,
                sizeof(pstSession->stOptions.achTerminalType) / sizeof(WCHAR),
                L"xterm");
    }

static int utssh_start_winsock(UTSSH_SESSION *pstSession)
    {
    WSADATA stWsa;

    if (pstSession->fWinsockStarted)
        return TRUE;

    if (WSAStartup(MAKEWORD(2, 2), &stWsa) != 0)
        {
        utssh_set_status(pstSession, L"Winsock initialization failed");
        return FALSE;
        }

    pstSession->fWinsockStarted = TRUE;
    return TRUE;
    }

static int utssh_connect_socket(UTSSH_SESSION *pstSession)
    {
    char achHost[256];
    char achPort[32];
    struct addrinfo stHints;
    struct addrinfo *pstResult;
    struct addrinfo *pstWalk;
    int iResult;

    if (pstSession->stOptions.achSshHost[0] == L'\0')
        {
        utssh_set_status(pstSession, L"SSH host is not configured");
        return FALSE;
        }

    utssh_wide_to_char(pstSession->stOptions.achSshHost, achHost,
            sizeof(achHost));
    wsprintfA(achPort, "%d", pstSession->stOptions.iSshPort);

    ZeroMemory(&stHints, sizeof(stHints));
    stHints.ai_family = AF_UNSPEC;
    stHints.ai_socktype = SOCK_STREAM;
    stHints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(achHost, achPort, &stHints, &pstResult);
    if (iResult != 0)
        {
        utssh_set_status(pstSession, L"SSH host name could not be resolved");
        return FALSE;
        }

    for (pstWalk = pstResult; pstWalk != 0; pstWalk = pstWalk->ai_next)
        {
        pstSession->s = socket(pstWalk->ai_family, pstWalk->ai_socktype,
                pstWalk->ai_protocol);
        if (pstSession->s == INVALID_SOCKET)
            continue;

        if (connect(pstSession->s, pstWalk->ai_addr,
                (int)pstWalk->ai_addrlen) == 0)
            break;

        closesocket(pstSession->s);
        pstSession->s = INVALID_SOCKET;
        }

    freeaddrinfo(pstResult);
    if (pstSession->s == INVALID_SOCKET)
        {
        utssh_set_status(pstSession, L"SSH TCP connection failed");
        return FALSE;
        }

    return TRUE;
    }

static int utssh_wait_socket(UTSSH_SESSION *pstSession, int fWrite,
        DWORD dwTimeout)
    {
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(pstSession->s, &fds);
    tv.tv_sec = (long)(dwTimeout / 1000);
    tv.tv_usec = (long)((dwTimeout % 1000) * 1000);
    r = select(0, fWrite ? 0 : &fds, fWrite ? &fds : 0, 0, &tv);
    return r > 0;
    }

static int utssh_flush_output(UTSSH_SESSION *pstSession, DWORD dwTimeout)
    {
    const u_char *pbOut;
    size_t cbOut;
    int cbSent;
    DWORD dwStart;
    int iError;

    dwStart = GetTickCount();
    for (;;)
        {
        pbOut = ssh_output_ptr(pstSession->ssh, &cbOut);
        if (cbOut == 0)
            return TRUE;
        cbSent = send(pstSession->s, (const char *)pbOut, (int)cbOut, 0);
        if (cbSent > 0)
            {
            ssh_output_consume(pstSession->ssh, (size_t)cbSent);
            continue;
            }
        iError = WSAGetLastError();
        if (iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS)
            {
            utssh_set_status(pstSession, L"SSH socket send failed");
            return FALSE;
            }
        if (GetTickCount() - dwStart >= dwTimeout)
            {
            utssh_set_status(pstSession, L"SSH socket send timed out");
            return FALSE;
            }
        utssh_wait_socket(pstSession, TRUE, 50);
        }
    }

static int utssh_read_input(UTSSH_SESSION *pstSession, DWORD dwTimeout)
    {
    BYTE abBuf[UTSSH_NET_CHUNK];
    int cbRead;
    int iError;

    if (!utssh_wait_socket(pstSession, FALSE, dwTimeout))
        return 0;
    cbRead = recv(pstSession->s, (char *)abBuf, sizeof(abBuf), 0);
    if (cbRead > 0)
        {
        if (ssh_input_append(pstSession->ssh, abBuf, (size_t)cbRead) != 0)
            {
            utssh_set_status(pstSession, L"SSH input buffer failed");
            return -1;
            }
        return cbRead;
        }
    if (cbRead == 0)
        {
        utssh_set_status(pstSession, L"SSH server closed the connection");
        return -1;
        }
    iError = WSAGetLastError();
    if (iError == WSAEWOULDBLOCK || iError == WSAEINPROGRESS)
        return 0;
    utssh_set_status(pstSession, L"SSH socket receive failed");
    return -1;
    }

static int utssh_verify_host_key(struct sshkey *pstKey, struct ssh *ssh)
    {
    UTSSH_SESSION *pstSession;
    struct hostkeys *pstHostKeys;
    const struct hostkey_entry *pstFound;
    HostStatus nStatus;
    char achHost[256];
    char achKnownHosts[MAX_PATH];
    char *pszFingerprint;
    WCHAR achMessage[1024];
    WCHAR achHostWide[256];
    WCHAR achFingerprintWide[256];
    WCHAR achKnownHostsWide[MAX_PATH];
    int nAnswer;

    pstSession = (UTSSH_SESSION *)ssh_get_app_data(ssh);
    if (pstSession == 0 || pstKey == 0)
        return -1;

    utssh_known_host_name(pstSession, achHost, sizeof(achHost));
    utssh_known_hosts_path(pstSession, achKnownHosts, sizeof(achKnownHosts));
    if (achHost[0] == '\0' || achKnownHosts[0] == '\0')
        {
        utssh_set_status(pstSession, L"SSH host key check is not configured");
        return -1;
        }

    pstHostKeys = init_hostkeys();
    if (pstHostKeys == 0)
        {
        utssh_set_status(pstSession, L"SSH host key memory allocation failed");
        return -1;
        }

    load_hostkeys(pstHostKeys, achHost, achKnownHosts, 0);
    pstFound = 0;
    nStatus = check_key_in_hostkeys(pstHostKeys, pstKey, &pstFound);
    free_hostkeys(pstHostKeys);

    if (nStatus == HOST_OK)
        {
        utssh_set_status(pstSession, L"SSH host key verified");
        return 0;
        }

    if (nStatus == HOST_CHANGED || nStatus == HOST_REVOKED)
        {
        utssh_set_status(pstSession,
                nStatus == HOST_REVOKED ?
                L"SSH host key is revoked; connection rejected" :
                L"SSH host key changed; connection rejected");
        return -1;
        }

    pszFingerprint = sshkey_fingerprint(pstKey, SSH_FP_HASH_DEFAULT,
            SSH_FP_DEFAULT);
    utssh_char_to_wide(achHost, achHostWide,
            sizeof(achHostWide) / sizeof(achHostWide[0]));
    utssh_char_to_wide(pszFingerprint != 0 ? pszFingerprint :
            "(fingerprint unavailable)", achFingerprintWide,
            sizeof(achFingerprintWide) / sizeof(achFingerprintWide[0]));
    utssh_char_to_wide(achKnownHosts, achKnownHostsWide,
            sizeof(achKnownHostsWide) / sizeof(achKnownHostsWide[0]));
    wsprintfW(achMessage,
            L"The SSH host key for %s is not in your known_hosts file.\n\n"
            L"Fingerprint: %s\n\n"
            L"Trust this host and add it to:\n%s",
            achHostWide, achFingerprintWide, achKnownHostsWide);
    nAnswer = MessageBoxW(NULL, achMessage,
            L"UltraTerminal SSH Host Key", MB_ICONWARNING | MB_YESNO |
            MB_DEFBUTTON2 | MB_TASKMODAL);

    if (pszFingerprint != 0)
        free(pszFingerprint);
    if (nAnswer != IDYES)
        {
        utssh_set_status(pstSession, L"SSH host key was rejected");
        return -1;
        }

    if (!add_host_to_hostfile(achKnownHosts, achHost, pstKey, 0))
        {
        utssh_set_status(pstSession,
                L"SSH host key accepted but could not be saved");
        return -1;
        }

    utssh_set_status(pstSession, L"SSH host key accepted and saved");
    return 0;
    }

static int utssh_next_packet(UTSSH_SESSION *pstSession, u_char *pbType,
        DWORD dwTimeout)
    {
    DWORD dwStart;
    DWORD dwElapsed;
    DWORD dwSlice;
    int r;

    dwStart = GetTickCount();
    for (;;)
        {
        if (!utssh_flush_output(pstSession, dwTimeout))
            return -1;
        *pbType = SSH_MSG_NONE;
        r = ssh_packet_next(pstSession->ssh, pbType);
        if (r != 0)
            {
            utssh_set_status_a(pstSession, ssh_err(r));
            return -1;
            }
        if (!utssh_flush_output(pstSession, dwTimeout))
            return -1;
        if (*pbType != SSH_MSG_NONE)
            return 1;
        if (dwTimeout == 0)
            return 0;
        dwElapsed = GetTickCount() - dwStart;
        if (dwElapsed >= dwTimeout)
            return 0;
        dwSlice = dwTimeout - dwElapsed;
        if (dwSlice > 100)
            dwSlice = 100;
        r = utssh_read_input(pstSession, dwSlice);
        if (r < 0)
            return -1;
        }
    }

static int utssh_send_service_request(UTSSH_SESSION *pstSession)
    {
    struct sshbuf *pstPayload;
    int r;

    pstPayload = sshbuf_new();
    if (pstPayload == 0)
        return FALSE;
    r = sshbuf_put_cstring(pstPayload, "ssh-userauth");
    if (r == 0)
        r = ssh_packet_put(pstSession->ssh, SSH2_MSG_SERVICE_REQUEST,
                sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));
    sshbuf_free(pstPayload);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }
    return TRUE;
    }

static int utssh_send_userauth_none(UTSSH_SESSION *pstSession,
        const char *pszUser)
    {
    struct sshbuf *pstPayload;
    int r;

    pstPayload = sshbuf_new();
    if (pstPayload == 0)
        return FALSE;
    r = sshbuf_put_cstring(pstPayload, pszUser);
    if (r == 0)
        r = sshbuf_put_cstring(pstPayload, "ssh-connection");
    if (r == 0)
        r = sshbuf_put_cstring(pstPayload, "none");
    if (r == 0)
        r = ssh_packet_put(pstSession->ssh, SSH2_MSG_USERAUTH_REQUEST,
                sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));
    sshbuf_free(pstPayload);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }
    return TRUE;
    }

static int utssh_send_userauth_pubkey(UTSSH_SESSION *pstSession,
        const char *pszUser, const char *pszIdentity)
    {
    struct sshkey *pstKey;
    struct sshbuf *pstSigData;
    struct sshbuf *pstPayload;
    u_char *pbBlob;
    size_t cbBlob;
    u_char *pbSig;
    size_t cbSig;
    char *pszComment;
    const char *pszAlg;
    char achPassphrase[512];
    char achPrompt[512];
    int r;

    pstKey = 0;
    pstSigData = 0;
    pstPayload = 0;
    pbBlob = 0;
    pbSig = 0;
    pszComment = 0;
    r = sshkey_load_private_type(KEY_UNSPEC, pszIdentity, "",
            &pstKey, &pszComment);
    if (r == SSH_ERR_KEY_WRONG_PASSPHRASE)
        {
        wsprintfA(achPrompt,
                "Enter the passphrase for SSH identity:\n%s",
                pszIdentity);
        if (utssh_prompt_secret(pstSession, pszIdentity, achPrompt, "",
                achPassphrase, sizeof(achPassphrase)))
            {
            r = sshkey_load_private_type(KEY_UNSPEC, pszIdentity,
                    achPassphrase, &pstKey, &pszComment);
            SecureZeroMemory(achPassphrase, sizeof(achPassphrase));
            }
        }
    free(pszComment);
    if (r != 0 || pstKey == 0)
        return FALSE;

    pszAlg = sshkey_ssh_name(pstKey);
    r = sshkey_to_blob(pstKey, &pbBlob, &cbBlob);
    if (r != 0)
        goto out;

    pstSigData = sshbuf_new();
    if (pstSigData == 0)
        {
        r = SSH_ERR_ALLOC_FAIL;
        goto out;
        }
    if ((r = sshbuf_put_stringb(pstSigData,
            pstSession->ssh->kex->session_id)) != 0 ||
            (r = sshbuf_put_u8(pstSigData,
            SSH2_MSG_USERAUTH_REQUEST)) != 0 ||
            (r = sshbuf_put_cstring(pstSigData, pszUser)) != 0 ||
            (r = sshbuf_put_cstring(pstSigData, "ssh-connection")) != 0 ||
            (r = sshbuf_put_cstring(pstSigData, "publickey")) != 0 ||
            (r = sshbuf_put_u8(pstSigData, 1)) != 0 ||
            (r = sshbuf_put_cstring(pstSigData, pszAlg)) != 0 ||
            (r = sshbuf_put_string(pstSigData, pbBlob, cbBlob)) != 0)
        goto out;
    r = sshkey_sign(pstKey, &pbSig, &cbSig, sshbuf_ptr(pstSigData),
            sshbuf_len(pstSigData), pszAlg, 0, 0, pstSession->ssh->compat);
    if (r != 0)
        goto out;

    pstPayload = sshbuf_new();
    if (pstPayload == 0)
        {
        r = SSH_ERR_ALLOC_FAIL;
        goto out;
        }
    if ((r = sshbuf_put_cstring(pstPayload, pszUser)) != 0 ||
            (r = sshbuf_put_cstring(pstPayload, "ssh-connection")) != 0 ||
            (r = sshbuf_put_cstring(pstPayload, "publickey")) != 0 ||
            (r = sshbuf_put_u8(pstPayload, 1)) != 0 ||
            (r = sshbuf_put_cstring(pstPayload, pszAlg)) != 0 ||
            (r = sshbuf_put_string(pstPayload, pbBlob, cbBlob)) != 0 ||
            (r = sshbuf_put_string(pstPayload, pbSig, cbSig)) != 0)
        goto out;
    r = ssh_packet_put(pstSession->ssh, SSH2_MSG_USERAUTH_REQUEST,
            sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));

out:
    sshbuf_free(pstSigData);
    sshbuf_free(pstPayload);
    free(pbBlob);
    freezero(pbSig, cbSig);
    sshkey_free(pstKey);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }
    return TRUE;
    }

static int utssh_send_userauth_password(UTSSH_SESSION *pstSession,
        const char *pszUser, const char *pszPassword)
    {
    struct sshbuf *pstPayload;
    int r;

    pstPayload = sshbuf_new();
    if (pstPayload == 0)
        return FALSE;
    r = sshbuf_put_cstring(pstPayload, pszUser);
    if (r == 0)
        r = sshbuf_put_cstring(pstPayload, "ssh-connection");
    if (r == 0)
        r = sshbuf_put_cstring(pstPayload, "password");
    if (r == 0)
        r = sshbuf_put_u8(pstPayload, 0);
    if (r == 0)
        r = sshbuf_put_cstring(pstPayload, pszPassword);
    if (r == 0)
        r = ssh_packet_put(pstSession->ssh, SSH2_MSG_USERAUTH_REQUEST,
                sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));
    sshbuf_free(pstPayload);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }
    return TRUE;
    }

static int utssh_wait_for_service_accept(UTSSH_SESSION *pstSession)
    {
    u_char bType;
    DWORD dwStart;
    int r;

    dwStart = GetTickCount();
    while (GetTickCount() - dwStart < UTSSH_CONNECT_TIMEOUT)
        {
        r = utssh_next_packet(pstSession, &bType, 1000);
        if (r < 0)
            return FALSE;
        if (r == 0)
            continue;
        if (bType == SSH2_MSG_SERVICE_ACCEPT)
            return TRUE;
        if (bType == SSH2_MSG_EXT_INFO || bType == SSH2_MSG_DEBUG ||
                bType == SSH2_MSG_IGNORE)
            continue;
        utssh_set_status(pstSession, L"Unexpected SSH packet before auth");
        return FALSE;
        }
    utssh_set_status(pstSession, L"SSH userauth service timed out");
    return FALSE;
    }

static int utssh_wait_for_auth_result(UTSSH_SESSION *pstSession)
    {
    u_char bType;
    char *pszMethods;
    u_char bPartial;
    DWORD dwStart;
    int r;

    dwStart = GetTickCount();
    while (GetTickCount() - dwStart < UTSSH_CONNECT_TIMEOUT)
        {
        r = utssh_next_packet(pstSession, &bType, 1000);
        if (r < 0)
            return -1;
        if (r == 0)
            continue;
        if (bType == SSH2_MSG_USERAUTH_SUCCESS)
            return 1;
        if (bType == SSH2_MSG_USERAUTH_FAILURE)
            {
            pszMethods = 0;
            bPartial = 0;
            sshpkt_get_cstring(pstSession->ssh, &pszMethods, 0);
            sshpkt_get_u8(pstSession->ssh, &bPartial);
            free(pszMethods);
            return 0;
            }
        if (bType == SSH2_MSG_USERAUTH_BANNER)
            continue;
        if (bType == SSH2_MSG_EXT_INFO || bType == SSH2_MSG_DEBUG ||
                bType == SSH2_MSG_IGNORE)
            continue;
        utssh_set_status(pstSession, L"Unexpected SSH auth packet");
        return -1;
        }
    utssh_set_status(pstSession, L"SSH authentication timed out");
    return -1;
    }

static int utssh_try_identity(UTSSH_SESSION *pstSession, const char *pszUser,
        const char *pszPath)
    {
    if (pszPath == 0 || pszPath[0] == '\0')
        return FALSE;
    if (!utssh_send_userauth_pubkey(pstSession, pszUser, pszPath))
        return FALSE;
    return utssh_wait_for_auth_result(pstSession) == 1;
    }

static int utssh_try_identity_list(UTSSH_SESSION *pstSession,
        const char *pszUser)
    {
    char achList[1024];
    char *pszStart;
    char *pszWalk;
    char chSave;
    char achIdentity[MAX_PATH];
    char achDefault[MAX_PATH];
    char achHome[MAX_PATH];
    DWORD cchHome;

    utssh_wide_to_char(pstSession->stOptions.achIdentityFiles, achList,
            sizeof(achList));
    pszStart = achList;
    while (pszStart != 0 && *pszStart != '\0')
        {
        while (*pszStart == ';' || *pszStart == ',' || *pszStart == ' ')
            ++pszStart;
        pszWalk = pszStart;
        while (*pszWalk != '\0' && *pszWalk != ';' && *pszWalk != ',')
            ++pszWalk;
        chSave = *pszWalk;
        *pszWalk = '\0';
        utssh_expand_user_path(pszStart, achIdentity, sizeof(achIdentity));
        if (utssh_try_identity(pstSession, pszUser, achIdentity))
            return TRUE;
        if (chSave == '\0')
            break;
        pszStart = pszWalk + 1;
        }

    cchHome = GetEnvironmentVariableA("USERPROFILE", achHome, sizeof(achHome));
    if (cchHome == 0 || cchHome >= sizeof(achHome))
        return FALSE;
    wsprintfA(achDefault, "%s\\.ssh\\id_ed25519", achHome);
    if (utssh_try_identity(pstSession, pszUser, achDefault))
        return TRUE;
    wsprintfA(achDefault, "%s\\.ssh\\id_rsa", achHome);
    if (utssh_try_identity(pstSession, pszUser, achDefault))
        return TRUE;
    return FALSE;
    }

static int utssh_try_password(UTSSH_SESSION *pstSession, const char *pszUser)
    {
    char achHost[256];
    char achTarget[384];
    char achPrompt[512];
    char achPassword[512];
    int nAuth;

    utssh_wide_to_char(pstSession->stOptions.achSshHost, achHost,
            sizeof(achHost));
    wsprintfA(achTarget, "%s@%s", pszUser, achHost);
    wsprintfA(achPrompt, "Enter the SSH password for %s.", achTarget);

    if (!utssh_prompt_secret(pstSession, achTarget, achPrompt, pszUser,
            achPassword, sizeof(achPassword)))
        return FALSE;

    if (!utssh_send_userauth_password(pstSession, pszUser, achPassword))
        {
        SecureZeroMemory(achPassword, sizeof(achPassword));
        return FALSE;
        }
    SecureZeroMemory(achPassword, sizeof(achPassword));

    nAuth = utssh_wait_for_auth_result(pstSession);
    return nAuth == 1;
    }

static int utssh_authenticate(UTSSH_SESSION *pstSession)
    {
    char achUser[128];
    int nAuth;

    utssh_wide_to_char(pstSession->stOptions.achSshUser, achUser,
            sizeof(achUser));
    if (achUser[0] == '\0')
        utssh_default_user(achUser, sizeof(achUser));

    if (!utssh_send_service_request(pstSession) ||
            !utssh_wait_for_service_accept(pstSession))
        return FALSE;

    if (!utssh_send_userauth_none(pstSession, achUser))
        return FALSE;
    nAuth = utssh_wait_for_auth_result(pstSession);
    if (nAuth == 1)
        {
        pstSession->fAuthenticated = TRUE;
        return TRUE;
        }
    if (nAuth < 0)
        return FALSE;

    if (utssh_try_identity_list(pstSession, achUser))
        {
        pstSession->fAuthenticated = TRUE;
        return TRUE;
        }

    if (utssh_try_password(pstSession, achUser))
        {
        pstSession->fAuthenticated = TRUE;
        return TRUE;
        }

    utssh_set_status(pstSession, L"SSH authentication failed");
    return FALSE;
    }

static int utssh_send_channel_open(UTSSH_SESSION *pstSession)
    {
    struct sshbuf *pstPayload;
    char achTarget[256];
    char achTerm[128];
    int r;

    pstPayload = sshbuf_new();
    if (pstPayload == 0)
        return FALSE;

    if (pstSession->stOptions.nMode == UTSSH_MODE_INTERACTIVE)
        {
        r = sshbuf_put_cstring(pstPayload, "session");
        }
    else
        {
        utssh_wide_to_char(pstSession->stOptions.achTargetHost, achTarget,
                sizeof(achTarget));
        r = sshbuf_put_cstring(pstPayload, "direct-tcpip");
        if (r == 0)
            r = sshbuf_put_u32(pstPayload, pstSession->uLocalChannel);
        if (r == 0)
            r = sshbuf_put_u32(pstPayload, UTSSH_CHANNEL_WINDOW);
        if (r == 0)
            r = sshbuf_put_u32(pstPayload, UTSSH_CHANNEL_PACKET);
        if (r == 0)
            r = sshbuf_put_cstring(pstPayload, achTarget);
        if (r == 0)
            r = sshbuf_put_u32(pstPayload,
                    (uint32_t)pstSession->stOptions.iTargetPort);
        if (r == 0)
            r = sshbuf_put_cstring(pstPayload, "127.0.0.1");
        if (r == 0)
            r = sshbuf_put_u32(pstPayload, 0);
        goto send_packet;
        }

    if (r == 0)
        r = sshbuf_put_u32(pstPayload, pstSession->uLocalChannel);
    if (r == 0)
        r = sshbuf_put_u32(pstPayload, UTSSH_CHANNEL_WINDOW);
    if (r == 0)
        r = sshbuf_put_u32(pstPayload, UTSSH_CHANNEL_PACKET);

send_packet:
    if (r == 0)
        r = ssh_packet_put(pstSession->ssh, SSH2_MSG_CHANNEL_OPEN,
                sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));
    sshbuf_free(pstPayload);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }

    if (pstSession->stOptions.nMode == UTSSH_MODE_INTERACTIVE)
        {
        utssh_wide_to_char(pstSession->stOptions.achTerminalType, achTerm,
                sizeof(achTerm));
        if (achTerm[0] == '\0')
            lstrcpynA(achTerm, "xterm", sizeof(achTerm));
        }
    return TRUE;
    }

static int utssh_send_channel_request(UTSSH_SESSION *pstSession,
        const char *pszRequest, struct sshbuf *pstExtra, int fWantReply)
    {
    struct sshbuf *pstPayload;
    int r;

    pstPayload = sshbuf_new();
    if (pstPayload == 0)
        return FALSE;
    r = sshbuf_put_u32(pstPayload, pstSession->uRemoteChannel);
    if (r == 0)
        r = sshbuf_put_cstring(pstPayload, pszRequest);
    if (r == 0)
        r = sshbuf_put_u8(pstPayload, fWantReply ? 1 : 0);
    if (r == 0 && pstExtra != 0)
        r = sshbuf_putb(pstPayload, pstExtra);
    if (r == 0)
        r = ssh_packet_put(pstSession->ssh, SSH2_MSG_CHANNEL_REQUEST,
                sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));
    sshbuf_free(pstPayload);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }
    return TRUE;
    }

static int utssh_start_shell(UTSSH_SESSION *pstSession)
    {
    struct sshbuf *pstPty;
    char achTerm[128];
    int r;

    pstPty = sshbuf_new();
    if (pstPty == 0)
        return FALSE;
    utssh_wide_to_char(pstSession->stOptions.achTerminalType, achTerm,
            sizeof(achTerm));
    if (achTerm[0] == '\0')
        lstrcpynA(achTerm, "xterm", sizeof(achTerm));
    r = sshbuf_put_cstring(pstPty, achTerm);
    if (r == 0)
        r = sshbuf_put_u32(pstPty, (uint32_t)pstSession->stOptions.iCols);
    if (r == 0)
        r = sshbuf_put_u32(pstPty, (uint32_t)pstSession->stOptions.iRows);
    if (r == 0)
        r = sshbuf_put_u32(pstPty, 0);
    if (r == 0)
        r = sshbuf_put_u32(pstPty, 0);
    if (r == 0)
        r = sshbuf_put_string(pstPty, "", 0);
    if (r != 0 || !utssh_send_channel_request(pstSession, "pty-req",
            pstPty, TRUE))
        {
        sshbuf_free(pstPty);
        return FALSE;
        }
    sshbuf_free(pstPty);
    return utssh_send_channel_request(pstSession, "shell", 0, TRUE);
    }

static int utssh_wait_channel_open(UTSSH_SESSION *pstSession)
    {
    u_char bType;
    uint32_t uRecipient;
    uint32_t uReason;
    DWORD dwStart;
    int r;

    dwStart = GetTickCount();
    while (GetTickCount() - dwStart < UTSSH_CONNECT_TIMEOUT)
        {
        r = utssh_next_packet(pstSession, &bType, 1000);
        if (r < 0)
            return FALSE;
        if (r == 0)
            continue;
        if (bType == SSH2_MSG_CHANNEL_OPEN_CONFIRMATION)
            {
            if (sshpkt_get_u32(pstSession->ssh, &uRecipient) != 0 ||
                    sshpkt_get_u32(pstSession->ssh,
                    &pstSession->uRemoteChannel) != 0 ||
                    sshpkt_get_u32(pstSession->ssh,
                    &pstSession->uRemoteWindow) != 0 ||
                    sshpkt_get_u32(pstSession->ssh,
                    &pstSession->uRemoteMaxPacket) != 0)
                return FALSE;
            if (uRecipient != pstSession->uLocalChannel)
                continue;
            pstSession->fChannelOpen = TRUE;
            if (pstSession->stOptions.nMode == UTSSH_MODE_INTERACTIVE &&
                    !utssh_start_shell(pstSession))
                return FALSE;
            return TRUE;
            }
        if (bType == SSH2_MSG_CHANNEL_OPEN_FAILURE)
            {
            sshpkt_get_u32(pstSession->ssh, &uRecipient);
            sshpkt_get_u32(pstSession->ssh, &uReason);
            utssh_set_status(pstSession, L"SSH channel open failed");
            return FALSE;
            }
        if (bType == SSH2_MSG_DEBUG || bType == SSH2_MSG_IGNORE)
            continue;
        }
    utssh_set_status(pstSession, L"SSH channel open timed out");
    return FALSE;
    }

static int utssh_handle_channel_packet(UTSSH_SESSION *pstSession, u_char bType)
    {
    uint32_t uRecipient;
    uint32_t uAdjust;
    u_char *pbData;
    size_t cbData;

    if (bType == SSH2_MSG_CHANNEL_DATA)
        {
        pbData = 0;
        cbData = 0;
        if (sshpkt_get_u32(pstSession->ssh, &uRecipient) != 0 ||
                sshpkt_get_string(pstSession->ssh, &pbData, &cbData) != 0)
            return FALSE;
        if (uRecipient == pstSession->uLocalChannel && cbData > 0)
            utssh_queue_bytes(pstSession, pbData, (int)cbData);
        free(pbData);
        return TRUE;
        }
    if (bType == SSH2_MSG_CHANNEL_EXTENDED_DATA)
        {
        pbData = 0;
        cbData = 0;
        sshpkt_get_u32(pstSession->ssh, &uRecipient);
        sshpkt_get_u32(pstSession->ssh, &uAdjust);
        sshpkt_get_string(pstSession->ssh, &pbData, &cbData);
        free(pbData);
        return TRUE;
        }
    if (bType == SSH2_MSG_CHANNEL_WINDOW_ADJUST)
        {
        if (sshpkt_get_u32(pstSession->ssh, &uRecipient) == 0 &&
                sshpkt_get_u32(pstSession->ssh, &uAdjust) == 0 &&
                uRecipient == pstSession->uLocalChannel)
            pstSession->uRemoteWindow += uAdjust;
        return TRUE;
        }
    if (bType == SSH2_MSG_CHANNEL_EOF || bType == SSH2_MSG_CHANNEL_CLOSE)
        {
        pstSession->fConnected = FALSE;
        utssh_set_status(pstSession, L"SSH channel closed");
        return TRUE;
        }
    if (bType == SSH2_MSG_CHANNEL_SUCCESS || bType == SSH2_MSG_CHANNEL_FAILURE)
        return TRUE;
    return TRUE;
    }

static int utssh_pump(UTSSH_SESSION *pstSession, DWORD dwTimeout)
    {
    u_char bType;
    int r;

    for (;;)
        {
        r = utssh_next_packet(pstSession, &bType, dwTimeout);
        if (r <= 0)
            return r == 0;
        if (bType == SSH2_MSG_IGNORE || bType == SSH2_MSG_DEBUG)
            continue;
        if (bType == SSH2_MSG_DISCONNECT)
            {
            pstSession->fConnected = FALSE;
            utssh_set_status(pstSession, L"SSH disconnected by server");
            return FALSE;
            }
        if (!utssh_handle_channel_packet(pstSession, bType))
            return FALSE;
        if (pstSession->cbRxUsed > 0 || dwTimeout == 0)
            return TRUE;
        }
    }

int utssh_connect(UTSSH_SESSION *pstSession)
    {
    u_long ulNonBlocking;
    u_char bType;
    DWORD dwStart;
    int r;

    if (pstSession == 0)
        return FALSE;

    utssh_disconnect(pstSession);
    utssh_apply_readconf(pstSession);
    if (!utssh_start_winsock(pstSession))
        return FALSE;

    if (!utssh_connect_socket(pstSession))
        return FALSE;

    r = ssh_init(&pstSession->ssh, 0, 0);
    if (r != 0)
        {
        utssh_set_status_a(pstSession, ssh_err(r));
        return FALSE;
        }
    ssh_set_app_data(pstSession->ssh, pstSession);
    ssh_set_verify_host_key_callback(pstSession->ssh, utssh_verify_host_key);

    dwStart = GetTickCount();
    while (GetTickCount() - dwStart < UTSSH_CONNECT_TIMEOUT)
        {
        r = utssh_next_packet(pstSession, &bType, 1000);
        if (r < 0)
            return FALSE;
        if (r == 0)
            continue;
        if (bType == SSH2_MSG_EXT_INFO || bType == SSH2_MSG_DEBUG ||
                bType == SSH2_MSG_IGNORE)
            continue;
        if (bType == SSH2_MSG_SERVICE_ACCEPT)
            break;
        if (bType >= SSH2_MSG_USERAUTH_MIN)
            break;
        }

    if (!utssh_authenticate(pstSession))
        return FALSE;
    if (!utssh_send_channel_open(pstSession) || !utssh_wait_channel_open(pstSession))
        return FALSE;

    ulNonBlocking = 1;
    ioctlsocket(pstSession->s, FIONBIO, &ulNonBlocking);
    pstSession->fConnected = TRUE;
    utssh_set_status(pstSession, L"Embedded OpenSSH channel connected");
    return TRUE;
    }

void utssh_disconnect(UTSSH_SESSION *pstSession)
    {
    if (pstSession == 0)
        return;

    if (pstSession->ssh != 0)
        {
        ssh_free(pstSession->ssh);
        pstSession->ssh = 0;
        }

    if (pstSession->s != INVALID_SOCKET)
        {
        shutdown(pstSession->s, SD_BOTH);
        closesocket(pstSession->s);
        pstSession->s = INVALID_SOCKET;
        }

    pstSession->fConnected = FALSE;
    pstSession->fAuthenticated = FALSE;
    pstSession->fChannelOpen = FALSE;
    pstSession->cbRxHead = 0;
    pstSession->cbRxTail = 0;
    pstSession->cbRxUsed = 0;
    }

int utssh_is_connected(const UTSSH_SESSION *pstSession)
    {
    return pstSession != 0 && pstSession->fConnected;
    }

int utssh_send(UTSSH_SESSION *pstSession, const void *pvData, int cbData)
    {
    struct sshbuf *pstPayload;
    int cbChunk;
    int cbSent;
    int r;

    if (pstSession == 0 || pvData == 0 || cbData < 0 ||
            !pstSession->fConnected || !pstSession->fChannelOpen)
        return -1;

    cbSent = 0;
    while (cbSent < cbData)
        {
        cbChunk = cbData - cbSent;
        if (cbChunk > (int)pstSession->uRemoteMaxPacket &&
                pstSession->uRemoteMaxPacket > 0)
            cbChunk = (int)pstSession->uRemoteMaxPacket;
        if (cbChunk > UTSSH_CHANNEL_PACKET)
            cbChunk = UTSSH_CHANNEL_PACKET;
        pstPayload = sshbuf_new();
        if (pstPayload == 0)
            return cbSent > 0 ? cbSent : -1;
        r = sshbuf_put_u32(pstPayload, pstSession->uRemoteChannel);
        if (r == 0)
            r = sshbuf_put_string(pstPayload,
                    (const BYTE *)pvData + cbSent, cbChunk);
        if (r == 0)
            r = ssh_packet_put(pstSession->ssh, SSH2_MSG_CHANNEL_DATA,
                    sshbuf_ptr(pstPayload), sshbuf_len(pstPayload));
        sshbuf_free(pstPayload);
        if (r != 0 || !utssh_flush_output(pstSession, 5000))
            return cbSent > 0 ? cbSent : -1;
        cbSent += cbChunk;
        }
    return cbSent;
    }

int utssh_receive(UTSSH_SESSION *pstSession, void *pvData, int cbData)
    {
    int cbRead;

    if (pstSession == 0 || pvData == 0 || cbData <= 0 ||
            !pstSession->fConnected)
        return 0;

    cbRead = utssh_pop_bytes(pstSession, (BYTE *)pvData, cbData);
    if (cbRead > 0)
        return cbRead;
    utssh_pump(pstSession, 0);
    return utssh_pop_bytes(pstSession, (BYTE *)pvData, cbData);
    }

void utssh_query_status(const UTSSH_SESSION *pstSession, WCHAR *pszStatus,
        int cchStatus)
    {
    if (pszStatus == 0 || cchStatus <= 0)
        return;

    if (pstSession == 0)
        {
        utssh_copy(pszStatus, cchStatus, L"OpenSSH runtime unavailable");
        return;
        }

    utssh_copy(pszStatus, cchStatus, pstSession->achStatus);
    }
