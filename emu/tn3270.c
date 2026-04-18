/*  File: tn3270.c
 *
 *  UltraTerminal TN3270/TN3270E emulator glue.
 *
 *  The protocol engine is kept in third_party/suite3270 so this module only
 *  adapts UltraTerminal settings, keyboard events, COM output, and rendering.
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
#include <third_party/suite3270/ultra_runtime.h>

#if defined(INCL_TN3270)

#define TN3270_HVIRTUAL_KEY 0x01000000

typedef struct stTn3270Private
    {
    UT3270_RUNTIME *runtime;
    UT3270_SNAPSHOT snapshot;
    } STTN3270;

static void tn3270_getscrsize(const HHEMU hhEmu, int *rows, int *cols);
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
static void tn3270_setscrsize(const HHEMU hhEmu);
#endif
static void tn3270_deinstall(const HHEMU hhEmu);
static int tn3270_kbdin(const HHEMU hhEmu, int kcode, const int fTest);
static void tn3270_render(const HHEMU hhEmu);
static int tn3270_send_runtime_output(const HHEMU hhEmu,
        unsigned char *buf, int len);
static int tn3270_key_action(int kcode, int *action, int *ch);
static void tn3270_settings_from_emu(const HHEMU hhEmu,
        UT3270_SETTINGS *settings);
static void tn3270_copy_wide(char *dst, int dst_len, const WCHAR *src);
static STATTR tn3270_attr_for_cell(const HHEMU hhEmu,
        const UT3270_CELL *cell);
static int tn3270_color_to_vc(unsigned char color, int fallback);
static int tn3270_query_snapshot(const HHEMU hhEmu, STTN3270 *pst);
static void tn3270_message(const HHEMU hhEmu, const char *message);

void tn3270_init(const HHEMU hhEmu)
    {
    STTN3270 *pst;
    UT3270_SETTINGS settings;

    if (hhEmu->pvPrivate)
        {
        tn3270_deinstall(hhEmu);
        }

    pst = (STTN3270 *)malloc(sizeof(STTN3270));
    if (pst == NULL)
        return;

    memset(pst, 0, sizeof(STTN3270));
    tn3270_settings_from_emu(hhEmu, &settings);
    if (ut3270_runtime_create(&pst->runtime, &settings) < 0)
        {
        free(pst);
        return;
        }

    hhEmu->pvPrivate = pst;
    hhEmu->emu_kbdin = tn3270_kbdin;
    hhEmu->emu_getscrsize = tn3270_getscrsize;
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    hhEmu->emu_setscrsize = tn3270_setscrsize;
#endif
    hhEmu->emu_deinstall = tn3270_deinstall;
    hhEmu->mode_block = SET;
    hhEmu->mode_protect = SET;

    tn3270_render(hhEmu);
    NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
    }

static void tn3270_getscrsize(const HHEMU hhEmu, int *rows, int *cols)
    {
    STTN3270 *pst;

    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst && tn3270_query_snapshot(hhEmu, pst) == 0)
        {
        *rows = pst->snapshot.rows;
        *cols = pst->snapshot.cols;
        }
    else
        {
        *rows = EMU_DEFAULT_ROWS;
        *cols = EMU_DEFAULT_COLS;
        }
    }

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
static void tn3270_setscrsize(const HHEMU hhEmu)
    {
    STTN3270 *pst;

    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || tn3270_query_snapshot(hhEmu, pst) < 0)
        return;

    hhEmu->emu_maxrow = pst->snapshot.rows - 1;
    hhEmu->emu_maxcol = pst->snapshot.cols - 1;
    }
#endif

static void tn3270_deinstall(const HHEMU hhEmu)
    {
    STTN3270 *pst;

    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst)
        {
        ut3270_runtime_destroy(pst->runtime);
        free(pst);
        hhEmu->pvPrivate = NULL;
        }
    }

int emuTn3270RecordIn(const HEMU hEmu, const unsigned char *buf, int len)
    {
    HHEMU hhEmu;
    STTN3270 *pst;
    unsigned char outbuf[UT3270_TX_LIMIT];
    int out_len;

    if (hEmu == 0 || buf == NULL || len <= 0)
        return -1;

    hhEmu = (HHEMU)hEmu;
    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    out_len = 0;
    if (ut3270_runtime_feed_record(pst->runtime, buf, len, outbuf,
                sizeof(outbuf), &out_len) < 0)
        return -1;

    if (out_len > 0)
        tn3270_send_runtime_output(hhEmu, outbuf, out_len);

    tn3270_render(hhEmu);
    return 0;
    }

void emuTn3270SetTelnetMode(const HEMU hEmu, int fTn3270E)
    {
    HHEMU hhEmu;
    STTN3270 *pst;

    if (hEmu == 0)
        return;

    hhEmu = (HHEMU)hEmu;
    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst && pst->runtime)
        ut3270_runtime_set_telnet_mode(pst->runtime,
                fTn3270E ? TRUE : FALSE);
    }

int emuTn3270StartPrinter(const HEMU hEmu)
    {
    HHEMU hhEmu;
    STTN3270 *pst;
    char message[256];

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    if (ut3270_runtime_start_printer(pst->runtime, "UltraTerminal3287.exe",
                message, sizeof(message)) < 0)
        {
        tn3270_message(hhEmu, message);
        return -1;
        }
    tn3270_message(hhEmu, message);
    tn3270_render(hhEmu);
    return 0;
    }

int emuTn3270StopPrinter(const HEMU hEmu)
    {
    HHEMU hhEmu;
    STTN3270 *pst;
    char message[256];

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    ut3270_runtime_stop_printer(pst->runtime, message, sizeof(message));
    tn3270_message(hhEmu, message);
    tn3270_render(hhEmu);
    return 0;
    }

int emuTn3270IndFileSend(const HEMU hEmu)
    {
    HHEMU hhEmu;
    STTN3270 *pst;
    char message[256];

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    ut3270_runtime_indfile_send(pst->runtime, NULL, NULL, message,
            sizeof(message));
    tn3270_message(hhEmu, message);
    tn3270_render(hhEmu);
    return 0;
    }

int emuTn3270IndFileReceive(const HEMU hEmu)
    {
    HHEMU hhEmu;
    STTN3270 *pst;
    char message[256];

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    ut3270_runtime_indfile_receive(pst->runtime, NULL, NULL, message,
            sizeof(message));
    tn3270_message(hhEmu, message);
    tn3270_render(hhEmu);
    return 0;
    }

static int tn3270_kbdin(const HHEMU hhEmu, int kcode, const int fTest)
    {
    STTN3270 *pst;
    int action;
    int ch;
    unsigned char outbuf[UT3270_TX_LIMIT];
    int out_len;

    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    if (tn3270_key_action(kcode, &action, &ch) < 0)
        return -1;

    if (fTest)
        return 0;

    out_len = 0;
    if (ut3270_runtime_key(pst->runtime, action, ch, outbuf, sizeof(outbuf),
                &out_len) == 0)
        {
        if (out_len > 0)
            tn3270_send_runtime_output(hhEmu, outbuf, out_len);
        tn3270_render(hhEmu);
        }
    else
        {
        tn3270_render(hhEmu);
        }

    return -1;
    }

static void tn3270_render(const HHEMU hhEmu)
    {
    STTN3270 *pst;
    int row;
    int col;
    int addr;
    STATTR attr;
    const UT3270_CELL *cell;
    int ch;

    pst = (STTN3270 *)hhEmu->pvPrivate;
    if (pst == NULL || tn3270_query_snapshot(hhEmu, pst) < 0)
        return;

    hhEmu->emu_maxrow = pst->snapshot.rows - 1;
    hhEmu->emu_maxcol = pst->snapshot.cols - 1;
    hhEmu->emu_currow = pst->snapshot.cursor_addr / pst->snapshot.cols;
    hhEmu->emu_curcol = pst->snapshot.cursor_addr % pst->snapshot.cols;

    for (row = 0; row < MAX_EMUROWS; ++row)
        {
        for (col = 0; col < MAX_EMUCOLS; ++col)
            {
            if (row < pst->snapshot.rows && col < pst->snapshot.cols)
                {
                addr = row * pst->snapshot.cols + col;
                cell = &pst->snapshot.cells[addr];
                attr = tn3270_attr_for_cell(hhEmu, cell);
                ch = (int)cell->ascii;
                if (attr.blank || cell->field_start)
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
        if (row < pst->snapshot.rows)
            hhEmu->emu_aiEnd[row] = pst->snapshot.cols - 1;
        else
            hhEmu->emu_aiEnd[row] = EMU_BLANK_LINE;
        }

    updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_maxrow);
    NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0);
    }

static int tn3270_send_runtime_output(const HHEMU hhEmu,
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

static int tn3270_key_action(int kcode, int *action, int *ch)
    {
    int vk;

    if (action == NULL || ch == NULL)
        return -1;

    *action = UT3270_KEY_NONE;
    *ch = 0;

    if (!(kcode & VIRTUAL_KEY))
        {
        if (kcode >= 0x20 && kcode <= 0x7e)
            {
            *action = UT3270_KEY_CHAR;
            *ch = kcode;
            return 0;
            }
        return -1;
        }

    vk = kcode & 0xffff;
    if ((kcode & TN3270_HVIRTUAL_KEY) && vk == 0x3b)
        {
        *action = UT3270_KEY_PA_BASE + 1;
        return 0;
        }

    if (vk >= VK_F1 && vk <= VK_F12)
        {
        if (kcode & CTRL_KEY)
            {
            if (vk <= VK_F3)
                {
                *action = UT3270_KEY_PA_BASE + (vk - VK_F1) + 1;
                return 0;
                }
            }
        if (kcode & SHIFT_KEY)
            *action = UT3270_KEY_PF_BASE + (vk - VK_F1) + 13;
        else
            *action = UT3270_KEY_PF_BASE + (vk - VK_F1) + 1;
        return 0;
        }

    if (vk >= VK_F13 && vk <= VK_F24)
        {
        *action = UT3270_KEY_PF_BASE + (vk - VK_F13) + 13;
        return 0;
        }

    switch (vk)
        {
    case VK_RETURN:
        *action = UT3270_KEY_ENTER;
        break;
    case VK_ESCAPE:
        *action = UT3270_KEY_CLEAR;
        break;
    case VK_CANCEL:
    case VK_SNAPSHOT:
        *action = UT3270_KEY_SYSREQ;
        break;
    case VK_TAB:
        *action = (kcode & SHIFT_KEY) ? UT3270_KEY_BACKTAB : UT3270_KEY_TAB;
        break;
    case VK_LEFT:
        *action = UT3270_KEY_LEFT;
        break;
    case VK_RIGHT:
        *action = UT3270_KEY_RIGHT;
        break;
    case VK_UP:
        *action = UT3270_KEY_UP;
        break;
    case VK_DOWN:
        *action = UT3270_KEY_DOWN;
        break;
    case VK_DELETE:
        *action = (kcode & CTRL_KEY) ? UT3270_KEY_ERASE_EOF :
                UT3270_KEY_DELETE;
        break;
    case VK_BACK:
        *action = UT3270_KEY_BACKSPACE;
        break;
    case VK_HOME:
        *action = UT3270_KEY_ERASE_FIELD;
        break;
    default:
        return -1;
        }

    return 0;
    }

static void tn3270_settings_from_emu(const HHEMU hhEmu,
        UT3270_SETTINGS *settings)
    {
    ut3270_settings_defaults(settings);
    settings->model = hhEmu->stUserSettings.nTn3270Model;
    settings->tls_mode = hhEmu->stUserSettings.nTn3270TlsMode;
    if (settings->tls_mode == UT3270_TLS_OFF &&
            hhEmu->stUserSettings.fTn3270Tls)
        settings->tls_mode = UT3270_TLS_DIRECT;
    settings->verify_tls = hhEmu->stUserSettings.fTn3270VerifyCert;
    settings->printer_enabled = hhEmu->stUserSettings.fTn3270Printer;
    tn3270_copy_wide(settings->lu_name, sizeof(settings->lu_name),
            hhEmu->stUserSettings.acTn3270LuName);
    tn3270_copy_wide(settings->device_name, sizeof(settings->device_name),
            hhEmu->stUserSettings.acTn3270DeviceName);
    tn3270_copy_wide(settings->host_codepage, sizeof(settings->host_codepage),
            hhEmu->stUserSettings.acTn3270HostCodePage);
    tn3270_copy_wide(settings->ca_path, sizeof(settings->ca_path),
            hhEmu->stUserSettings.acTn3270CaPath);
    tn3270_copy_wide(settings->cert_path, sizeof(settings->cert_path),
            hhEmu->stUserSettings.acTn3270CertPath);
    tn3270_copy_wide(settings->key_path, sizeof(settings->key_path),
            hhEmu->stUserSettings.acTn3270KeyPath);
    tn3270_copy_wide(settings->indfile_dir, sizeof(settings->indfile_dir),
            hhEmu->stUserSettings.acTn3270IndFileDir);
    tn3270_copy_wide(settings->indfile_options,
            sizeof(settings->indfile_options),
            hhEmu->stUserSettings.acTn3270IndFileOptions);
    tn3270_copy_wide(settings->printer_lu, sizeof(settings->printer_lu),
            hhEmu->stUserSettings.acTn3270PrinterLu);
    tn3270_copy_wide(settings->printer_options,
            sizeof(settings->printer_options),
            hhEmu->stUserSettings.acTn3270PrinterOptions);
    }

static void tn3270_copy_wide(char *dst, int dst_len, const WCHAR *src)
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

static STATTR tn3270_attr_for_cell(const HHEMU hhEmu,
        const UT3270_CELL *cell)
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

    fg = tn3270_color_to_vc(cell->foreground, -1);
    bg = tn3270_color_to_vc(cell->background, -1);
    if (fg >= 0)
        attr.txtclr = (unsigned int)fg;
    if (bg >= 0)
        attr.bkclr = (unsigned int)bg;

    attr.protect = UT3270_FA_IS_PROTECTED(cell->field_attr) ? 1 : 0;
    attr.hilite = UT3270_FA_IS_HIGH(cell->field_attr) ? 1 : 0;
    attr.blank = UT3270_FA_IS_ZERO(cell->field_attr) ? 1 : 0;

    switch (cell->highlight)
        {
    case UT3270_XAH_REVERSE:
        attr.revvid = 1;
        break;
    case UT3270_XAH_UNDERSCORE:
        attr.undrln = 1;
        break;
    case UT3270_XAH_BLINK:
        attr.blink = 1;
        break;
    case UT3270_XAH_INTENSIFY:
        attr.hilite = 1;
        break;
    default:
        break;
        }

    if (cell->field_start)
        attr.blank = 1;

    return attr;
    }

static int tn3270_color_to_vc(unsigned char color, int fallback)
    {
    switch (color)
        {
    case UT3270_COLOR_BLUE: return VC_BLUE;
    case UT3270_COLOR_RED: return VC_RED;
    case UT3270_COLOR_PINK: return VC_BRT_MAGENTA;
    case UT3270_COLOR_GREEN: return VC_GREEN;
    case UT3270_COLOR_TURQUOISE: return VC_CYAN;
    case UT3270_COLOR_YELLOW: return VC_BRT_YELLOW;
    case UT3270_COLOR_BLACK:
    case UT3270_COLOR_NEUTRAL_BLACK: return VC_BLACK;
    case UT3270_COLOR_DEEP_BLUE: return VC_BRT_BLUE;
    case UT3270_COLOR_ORANGE: return VC_BRT_RED;
    case UT3270_COLOR_PURPLE: return VC_MAGENTA;
    case UT3270_COLOR_PALE_GREEN: return VC_BRT_GREEN;
    case UT3270_COLOR_PALE_TURQUOISE: return VC_BRT_CYAN;
    case UT3270_COLOR_GREY: return VC_GRAY;
    case UT3270_COLOR_WHITE:
    case UT3270_COLOR_NEUTRAL_WHITE: return VC_BRT_WHITE;
    default: return fallback;
        }
    }

static int tn3270_query_snapshot(const HHEMU hhEmu, STTN3270 *pst)
    {
    (void)hhEmu;
    if (pst == NULL || pst->runtime == NULL)
        return -1;
    return ut3270_runtime_snapshot(pst->runtime, &pst->snapshot);
    }

static void tn3270_message(const HHEMU hhEmu, const char *message)
    {
    if (message == NULL || message[0] == '\0')
        return;
    ut_message_box_utf8(sessQueryHwnd(hhEmu->hSession), message,
            L"UltraTerminal TN3270", MB_OK | MB_ICONINFORMATION);
    }

#endif
