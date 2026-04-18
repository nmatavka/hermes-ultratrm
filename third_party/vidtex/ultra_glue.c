/*
 * UltraTerminal Videotex glue marker.
 *
 * Rendering glue lives in emu/videotex.c so it can access HEMU internals.
 * This target keeps the CMake/library boundary explicit for downstream
 * build tooling and license scanners.
 */

#include "ultra_vidtex.h"

const char *utvideotex_ultra_glue_name(void)
    {
    return "UltraTerminal Videotex";
    }
