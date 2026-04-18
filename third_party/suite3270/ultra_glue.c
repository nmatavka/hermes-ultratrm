/*
 * UltraTerminal glue anchor for the embedded suite3270-derived runtime.
 *
 * The active display, keyboard, status, transport, and printer hooks live in
 * emu/tn3270.c and ultra_runtime.c. This small unit gives CMake a distinct
 * suite3270_ultra_glue library boundary, matching the upstream-style split and
 * keeping future GUI/timer/task hook expansion out of the protocol core.
 */

#include "ultra_runtime.h"

int suite3270_ultra_glue_anchor(void)
    {
    return UT3270_MODEL_DEFAULT;
    }
