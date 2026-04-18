/*
 * UltraTerminal 3287 Printer helper.
 *
 * This target gives TN3270 sessions a sibling Windows PE process to launch
 * for associated printer work. Protocol constants and notices are kept with
 * the suite3270-derived code in this directory.
 */

#include <windows.h>

#include "ultra_pr3287.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)nCmdShow;

    return ut3287_run(lpCmdLine);
}
