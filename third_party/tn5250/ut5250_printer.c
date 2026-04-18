/*
 * UltraTerminal 5250 Printer helper executable.
 */
#include <windows.h>

#include "ultra_printer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)nCmdShow;

    return ut5250_printer_run(lpCmdLine);
}
