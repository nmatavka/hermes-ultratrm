#ifndef ULTRATERMINAL_UNIMODEM_H
#define ULTRATERMINAL_UNIMODEM_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal Unimodem compatibility declarations for lineGetDevConfig/
 * lineSetDevConfig on comm/datamodem devices.
 */

#define MDMCFG_VERSION 0x00010003

#define TERMINAL_PRE  0x0001
#define TERMINAL_POST 0x0002
#define MANUAL_DIAL   0x0004
#define LAUNCH_LIGHTS 0x0008

typedef struct tagDEVCFGHDR {
    DWORD dwSize;
    DWORD dwVersion;
    WORD fwOptions;
    WORD wWaitBong;
} DEVCFGHDR;

typedef DEVCFGHDR UMDEVCFGHDR;
typedef UMDEVCFGHDR *PUMDEVCFGHDR;

typedef struct tagDEVCFG {
    DEVCFGHDR dfgHdr;
    COMMCONFIG commconfig;
} DEVCFG;

typedef DEVCFG UMDEVCFG;
typedef UMDEVCFG *PUMDEVCFG;

#ifdef __cplusplus
}
#endif

#endif
