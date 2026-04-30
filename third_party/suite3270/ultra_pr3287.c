/*
 * UltraTerminal 3287 Printer helper runtime.
 *
 * This keeps the sibling Windows PE helper process and command-line contract
 * in place for associated printer sessions. Full 3287 SCS print rendering is
 * intentionally isolated behind this boundary so the suite3270 pr3287 code can
 * replace the minimal status helper without changing UltraTerminal.dll.
 */

#include "ultra_pr3287.h"

#include <windows.h>
#include <string.h>

static int has_switch(const char *cmdline, const char *sw)
    {
    if (cmdline == NULL || sw == NULL)
        return 0;
    return strstr(cmdline, sw) != NULL;
    }

int ut3287_run(const char *cmdline)
    {
    if (has_switch(cmdline, "--quiet"))
        return 0;

    MessageBoxW(NULL,
        L"UltraTerminal 3287 Printer helper is installed and accepts "
        L"IBM 3270 associated-printer launch options. Printing availability "
        L"depends on the host LU and Wine/Windows printer support.",
        L"UltraTerminal 3287 Printer",
        MB_OK | MB_ICONINFORMATION);
    return 0;
    }
