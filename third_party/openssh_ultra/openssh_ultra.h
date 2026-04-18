#if !defined(ULTRATERMINAL_OPENSSH_ULTRA_H)
#define ULTRATERMINAL_OPENSSH_ULTRA_H

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define UTSSH_MODE_INTERACTIVE  1
#define UTSSH_MODE_TUNNEL       2

#define UTSSH_STATUS_CHARS      256

typedef struct utssh_session UTSSH_SESSION;

typedef struct utssh_options
    {
    int nMode;
    int iSshPort;
    int iTargetPort;
    int fAgent;
    int fCompression;
    WCHAR achSshHost[128];
    WCHAR achSshUser[128];
    WCHAR achConfigPath[MAX_PATH];
    WCHAR achKnownHostsPath[MAX_PATH];
    WCHAR achIdentityFiles[512];
    WCHAR achTargetHost[128];
    WCHAR achProxy[512];
    WCHAR achForwardings[512];
    WCHAR achTerminalType[64];
    int iRows;
    int iCols;
    } UTSSH_OPTIONS;

UTSSH_SESSION *utssh_create(void);
void utssh_destroy(UTSSH_SESSION *pstSession);
void utssh_set_options(UTSSH_SESSION *pstSession,
        const UTSSH_OPTIONS *pstOptions);
int utssh_connect(UTSSH_SESSION *pstSession);
void utssh_disconnect(UTSSH_SESSION *pstSession);
int utssh_is_connected(const UTSSH_SESSION *pstSession);
int utssh_send(UTSSH_SESSION *pstSession, const void *pvData, int cbData);
int utssh_receive(UTSSH_SESSION *pstSession, void *pvData, int cbData);
void utssh_query_status(const UTSSH_SESSION *pstSession, WCHAR *pszStatus,
        int cchStatus);

#if defined(__cplusplus)
}
#endif

#endif
