/*  File: videotex.c
 *
 *  UltraTerminal Videotex/Viewdata-over-IP emulator glue.
 */

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>
#include <string.h>

#include <tdll/stdtyp.h>
#include <tdll/session.h>
#include <tdll/com.h>
#include <tdll/update.h>
#include <tdll/ut_text.h>
#include <tdll/chars.h>
#include <tdll/assert.h>

#include "emu.h"
#include "emu.hh"
#include <third_party/vidtex/ultra_vidtex.h>

#if defined(INCL_VIDEOTEX)

typedef struct stVideotexPrivate
    {
    UTVIDTEX_RUNTIME *runtime;
    UTVIDTEX_SNAPSHOT snapshot;
    } STVIDEOTEX;

static void videotex_getscrsize(const HHEMU hhEmu, int *rows, int *cols);
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
static void videotex_setscrsize(const HHEMU hhEmu);
#endif
static void videotex_deinstall(const HHEMU hhEmu);
static int videotex_kbdin(const HHEMU hhEmu, int kcode, const int fTest);
static void videotex_notify(const HHEMU hhEmu, const int nEvent);
static void videotex_render(const HHEMU hhEmu);
static int videotex_send_runtime_output(const HHEMU hhEmu,
        unsigned char *buf, int len);
static int videotex_key_to_byte(const HHEMU hhEmu, int kcode,
        int *out_key, int *is_local_action);
static void videotex_settings_from_emu(const HHEMU hhEmu,
        UTVIDTEX_SETTINGS *settings);
static void videotex_copy_wide(char *dst, int dst_len, const WCHAR *src);
static STATTR videotex_attr_for_cell(const HHEMU hhEmu,
        const UTVIDTEX_SNAPSHOT *snapshot, const UTVIDTEX_CELL *cell);
static int videotex_color_to_vc(unsigned char color, int fallback);
static int videotex_query_snapshot(const HHEMU hhEmu, STVIDEOTEX *pst);
static void videotex_message(const HHEMU hhEmu, const char *message);

void videotex_init(const HHEMU hhEmu)
    {
    STVIDEOTEX *pst;
    UTVIDTEX_SETTINGS settings;

    if (hhEmu->pvPrivate)
        videotex_deinstall(hhEmu);

    pst = (STVIDEOTEX *)malloc(sizeof(STVIDEOTEX));
    if (pst == NULL)
        return;

    memset(pst, 0, sizeof(STVIDEOTEX));
    videotex_settings_from_emu(hhEmu, &settings);
    if (utvideotex_runtime_create(&pst->runtime, &settings) < 0)
        {
        free(pst);
        return;
        }

    hhEmu->pvPrivate = pst;
    hhEmu->emu_kbdin = videotex_kbdin;
    hhEmu->emu_getscrsize = videotex_getscrsize;
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    hhEmu->emu_setscrsize = videotex_setscrsize;
#endif
    hhEmu->emu_deinstall = videotex_deinstall;
    hhEmu->emu_ntfy = videotex_notify;
    hhEmu->mode_block = RESET;
    hhEmu->mode_protect = RESET;

    videotex_render(hhEmu);
    NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
    }

static void videotex_getscrsize(const HHEMU hhEmu, int *rows, int *cols)
    {
    (void)hhEmu;
    *rows = UTVIDTEX_ROWS;
    *cols = UTVIDTEX_COLS;
    }

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
static void videotex_setscrsize(const HHEMU hhEmu)
    {
    hhEmu->emu_maxrow = UTVIDTEX_ROWS - 1;
    hhEmu->emu_maxcol = UTVIDTEX_COLS - 1;
    }
#endif

static void videotex_deinstall(const HHEMU hhEmu)
    {
    STVIDEOTEX *pst;

    pst = (STVIDEOTEX *)hhEmu->pvPrivate;
    if (pst)
        {
        utvideotex_runtime_destroy(pst->runtime);
        free(pst);
        hhEmu->pvPrivate = NULL;
        }
    }

int emuVideotexBytesIn(const HEMU hEmu, const unsigned char *buf, int len)
    {
    HHEMU hhEmu;
    STVIDEOTEX *pst;

    if (hEmu == 0 || buf == NULL || len <= 0)
        return -1;

    hhEmu = (HHEMU)hEmu;
    pst = (STVIDEOTEX *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    if (utvideotex_runtime_feed(pst->runtime, buf, len) < 0)
        return -1;

    videotex_render(hhEmu);
    return 0;
    }

int emuVideotexShowSettings(const HEMU hEmu)
    {
    HHEMU hhEmu;

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    videotex_message(hhEmu,
            "Videotex settings are stored with the active UltraTerminal "
            "session. Built-in profiles include NXTel, TeeFax, Telstar, "
            "CCL4, and Night Owl.");
    return 0;
    }

static void videotex_notify(const HHEMU hhEmu, const int nEvent)
    {
    STVIDEOTEX *pst;
    unsigned char outbuf[UTVIDTEX_TX_LIMIT];
    int out_len;

    pst = (STVIDEOTEX *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return;

    if (nEvent == EMU_EVENT_CONNECTED)
        {
        out_len = 0;
        if (utvideotex_runtime_connected(pst->runtime, outbuf,
                    sizeof(outbuf), &out_len) == 0 && out_len > 0)
            videotex_send_runtime_output(hhEmu, outbuf, out_len);
        videotex_render(hhEmu);
        }
    }

static int videotex_kbdin(const HHEMU hhEmu, int kcode, const int fTest)
    {
    STVIDEOTEX *pst;
    unsigned char outbuf[UTVIDTEX_TX_LIMIT];
    int out_len;
    int key;
    int is_local_action;

    pst = (STVIDEOTEX *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    if (videotex_key_to_byte(hhEmu, kcode, &key, &is_local_action) < 0)
        return -1;

    if (fTest)
        return 0;

    if (is_local_action == 1)
        {
        utvideotex_runtime_toggle_reveal(pst->runtime);
        videotex_render(hhEmu);
        return -1;
        }
    if (is_local_action == 2)
        {
        utvideotex_runtime_toggle_flash(pst->runtime);
        videotex_render(hhEmu);
        return -1;
        }
    if (is_local_action == 3)
        {
        emuVideotexShowSettings((HEMU)hhEmu);
        return -1;
        }

    out_len = 0;
    if (utvideotex_runtime_key(pst->runtime, key, outbuf, sizeof(outbuf),
                &out_len) == 0)
        {
        if (out_len > 0)
            videotex_send_runtime_output(hhEmu, outbuf, out_len);
        }

    return -1;
    }

static void videotex_render(const HHEMU hhEmu)
    {
    STVIDEOTEX *pst;
    int row;
    int col;
    int addr;
    STATTR attr;
    const UTVIDTEX_CELL *cell;
    int ch;

    pst = (STVIDEOTEX *)hhEmu->pvPrivate;
    if (pst == NULL || videotex_query_snapshot(hhEmu, pst) < 0)
        return;

    hhEmu->emu_maxrow = UTVIDTEX_ROWS - 1;
    hhEmu->emu_maxcol = UTVIDTEX_COLS - 1;
    hhEmu->emu_currow = pst->snapshot.cursor_row;
    hhEmu->emu_curcol = pst->snapshot.cursor_col;

    for (row = 0; row < MAX_EMUROWS; ++row)
        {
        for (col = 0; col < MAX_EMUCOLS; ++col)
            {
            if (row < UTVIDTEX_ROWS && col < UTVIDTEX_COLS)
                {
                addr = row * UTVIDTEX_COLS + col;
                cell = &pst->snapshot.cells[addr];
                attr = videotex_attr_for_cell(hhEmu, &pst->snapshot, cell);
                ch = (int)cell->ch;
                if (attr.blank)
                    ch = ' ';
                hhEmu->emu_apText[row][col] = (ECHAR)ch;
                hhEmu->emu_apAttr[row][col] = attr;
                }
            else
                {
                hhEmu->emu_apText[row][col] = EMU_BLANK_CHAR;
                hhEmu->emu_apAttr[row][col] = hhEmu->emu_clearattr;
                }
            }
        if (row < UTVIDTEX_ROWS)
            hhEmu->emu_aiEnd[row] = UTVIDTEX_COLS - 1;
        else
            hhEmu->emu_aiEnd[row] = EMU_BLANK_LINE;
        }

    updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_maxrow);
    NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0);
    }

static int videotex_send_runtime_output(const HHEMU hhEmu,
        unsigned char *buf, int len)
    {
    HCOM hCom;

    if (buf == NULL || len <= 0)
        return -1;

    hCom = sessQueryComHdl(hhEmu->hSession);
    if (hCom == 0)
        return -1;

    return ComSndBufrSend(hCom, buf, len, 1);
    }

static int videotex_key_to_byte(const HHEMU hhEmu, int kcode,
        int *out_key, int *is_local_action)
    {
    int vk;

    (void)hhEmu;
    if (out_key == NULL || is_local_action == NULL)
        return -1;

    *out_key = 0;
    *is_local_action = 0;

    if (!(kcode & VIRTUAL_KEY))
        {
        if (kcode == 0x12)
            {
            *is_local_action = 1;
            return 0;
            }
        if (kcode == 0x02)
            {
            *is_local_action = 2;
            return 0;
            }
        if (kcode >= 0x20 && kcode <= 0x7e)
            {
            *out_key = kcode;
            return 0;
            }
        return -1;
        }

    vk = kcode & 0xffff;
    switch (vk)
        {
    case VK_RETURN:
        *out_key = '\r';
        break;
    case VK_BACK:
        *out_key = '\b';
        break;
    case VK_ESCAPE:
        *out_key = 0x1b;
        break;
    case VK_F5:
        *is_local_action = 1;
        break;
    case VK_F6:
        *is_local_action = 2;
        break;
    case VK_F7:
        *is_local_action = 3;
        break;
    default:
        return -1;
        }

    return 0;
    }

static void videotex_settings_from_emu(const HHEMU hhEmu,
        UTVIDTEX_SETTINGS *settings)
    {
    const UTVIDTEX_PROFILE *profile;

    utvideotex_settings_defaults(settings);
    videotex_copy_wide(settings->profile_name, sizeof(settings->profile_name),
            hhEmu->stUserSettings.acVideotexProfile);
    profile = utvideotex_profile_by_name(settings->profile_name);
    if (profile)
        utvideotex_settings_apply_profile(settings, profile);

    if (hhEmu->stUserSettings.acVideotexHost[0] != L'\0')
        videotex_copy_wide(settings->host, sizeof(settings->host),
                hhEmu->stUserSettings.acVideotexHost);
    if (hhEmu->stUserSettings.nVideotexPort > 0)
        settings->port = hhEmu->stUserSettings.nVideotexPort;
    settings->font_map = hhEmu->stUserSettings.nVideotexFontMap;
    settings->telnet_compat = hhEmu->stUserSettings.fVideotexTelnetCompat;
    }

static void videotex_copy_wide(char *dst, int dst_len, const WCHAR *src)
    {
    if (dst == NULL || dst_len <= 0)
        return;
    dst[0] = '\0';
    if (src == NULL)
        return;

#if defined(UNICODE)
    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, dst_len, NULL, NULL);
    dst[dst_len - 1] = '\0';
#else
    strncpy(dst, src, (size_t)(dst_len - 1));
    dst[dst_len - 1] = '\0';
#endif
    }

static STATTR videotex_attr_for_cell(const HHEMU hhEmu,
        const UTVIDTEX_SNAPSHOT *snapshot, const UTVIDTEX_CELL *cell)
    {
    STATTR attr;
    int fg;
    int bg;

    memset(&attr, 0, sizeof(attr));
#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
    attr.txtclr = (unsigned int)(hhEmu->stUserSettings.nTextColor & 0x0f);
    attr.bkclr = (unsigned int)(hhEmu->stUserSettings.nBackgroundColor & 0x0f);
#else
    attr.txtclr = VC_WHITE;
    attr.bkclr = VC_BLACK;
#endif

    fg = videotex_color_to_vc(cell->foreground, attr.txtclr);
    bg = videotex_color_to_vc(cell->background, attr.bkclr);
    attr.txtclr = (unsigned int)(fg & 0x0f);
    attr.bkclr = (unsigned int)(bg & 0x0f);
    attr.hilite = (fg >= VC_GRAY) ? 1 : 0;
    attr.blink = (cell->flags & UTVIDTEX_CELL_FLASH) ? 1 : 0;
    attr.blank = 0;
    if ((cell->flags & UTVIDTEX_CELL_CONCEAL) && !snapshot->reveal_on)
        attr.blank = 1;
    if ((cell->flags & UTVIDTEX_CELL_FLASH) && !snapshot->flash_on)
        attr.blank = 1;
    if (cell->flags & UTVIDTEX_CELL_DOUBLE_TOP)
        attr.dblhilo = 1;
    if (cell->flags & UTVIDTEX_CELL_DOUBLE_BOTTOM)
        attr.dblhihi = 1;

    return attr;
    }

static int videotex_color_to_vc(unsigned char color, int fallback)
    {
    switch (color)
        {
    case UTVIDTEX_BLACK:
        return VC_BLACK;
    case UTVIDTEX_RED:
        return VC_RED;
    case UTVIDTEX_GREEN:
        return VC_GREEN;
    case UTVIDTEX_YELLOW:
        return VC_BROWN;
    case UTVIDTEX_BLUE:
        return VC_BLUE;
    case UTVIDTEX_MAGENTA:
        return VC_MAGENTA;
    case UTVIDTEX_CYAN:
        return VC_CYAN;
    case UTVIDTEX_WHITE:
        return VC_WHITE;
    default:
        return fallback;
        }
    }

static int videotex_query_snapshot(const HHEMU hhEmu, STVIDEOTEX *pst)
    {
    (void)hhEmu;
    if (pst == NULL || pst->runtime == NULL)
        return -1;
    return utvideotex_runtime_snapshot(pst->runtime, &pst->snapshot);
    }

static void videotex_message(const HHEMU hhEmu, const char *message)
    {
    if (message == NULL || message[0] == '\0')
        return;
    ut_message_box_utf8(sessQueryHwnd(hhEmu->hSession), message,
            L"UltraTerminal Videotex", MB_OK | MB_ICONINFORMATION);
    }

#endif
