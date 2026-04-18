/*  File: tn5250.c
 *
 *  UltraTerminal IBM 5250/TN5250 emulator glue.
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
#include <third_party/tn5250/ultra_runtime.h>

#if defined(INCL_TN5250)

typedef struct stTn5250Private
    {
    UT5250_RUNTIME *runtime;
    UT5250_SNAPSHOT snapshot;
    } STTN5250;

static void tn5250_getscrsize(const HHEMU hhEmu, int *rows, int *cols);
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
static void tn5250_setscrsize(const HHEMU hhEmu);
#endif
static void tn5250_deinstall(const HHEMU hhEmu);
static int tn5250_kbdin(const HHEMU hhEmu, int kcode, const int fTest);
static void tn5250_notify(const HHEMU hhEmu, const int nEvent);
static void tn5250_render(const HHEMU hhEmu);
static int tn5250_send_runtime_output(const HHEMU hhEmu,
        unsigned char *buf, int len);
static int tn5250_key_action(int kcode, int *action, int *ch);
static void tn5250_settings_from_emu(const HHEMU hhEmu,
        UT5250_SETTINGS *settings);
static void tn5250_copy_wide(char *dst, int dst_len, const WCHAR *src);
static STATTR tn5250_attr_for_cell(const HHEMU hhEmu,
        const UT5250_CELL *cell);
static int tn5250_query_snapshot(const HHEMU hhEmu, STTN5250 *pst);
static void tn5250_message(const HHEMU hhEmu, const char *message);

void tn5250_init(const HHEMU hhEmu)
    {
    STTN5250 *pst;
    UT5250_SETTINGS settings;

    if (hhEmu->pvPrivate)
        tn5250_deinstall(hhEmu);

    pst = (STTN5250 *)malloc(sizeof(STTN5250));
    if (pst == NULL)
        return;

    memset(pst, 0, sizeof(STTN5250));
    tn5250_settings_from_emu(hhEmu, &settings);
    if (ut5250_runtime_create(&pst->runtime, &settings) < 0)
        {
        free(pst);
        return;
        }

    hhEmu->pvPrivate = pst;
    hhEmu->emu_kbdin = tn5250_kbdin;
    hhEmu->emu_getscrsize = tn5250_getscrsize;
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    hhEmu->emu_setscrsize = tn5250_setscrsize;
#endif
    hhEmu->emu_deinstall = tn5250_deinstall;
    hhEmu->emu_ntfy = tn5250_notify;
    hhEmu->mode_block = SET;
    hhEmu->mode_protect = SET;

    tn5250_render(hhEmu);
    NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
    }

static void tn5250_getscrsize(const HHEMU hhEmu, int *rows, int *cols)
    {
    STTN5250 *pst;

    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst && tn5250_query_snapshot(hhEmu, pst) == 0)
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
static void tn5250_setscrsize(const HHEMU hhEmu)
    {
    STTN5250 *pst;

    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || tn5250_query_snapshot(hhEmu, pst) < 0)
        return;

    hhEmu->emu_maxrow = pst->snapshot.rows - 1;
    hhEmu->emu_maxcol = pst->snapshot.cols - 1;
    }
#endif

static void tn5250_deinstall(const HHEMU hhEmu)
    {
    STTN5250 *pst;

    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst)
        {
        ut5250_runtime_destroy(pst->runtime);
        free(pst);
        hhEmu->pvPrivate = NULL;
        }
    }

int emuTn5250BytesIn(const HEMU hEmu, const unsigned char *buf, int len)
    {
    HHEMU hhEmu;
    STTN5250 *pst;
    unsigned char outbuf[UT5250_TX_LIMIT];
    int out_len;

    if (hEmu == 0 || buf == NULL || len <= 0)
        return -1;

    hhEmu = (HHEMU)hEmu;
    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    out_len = 0;
    if (ut5250_runtime_feed_bytes(pst->runtime, buf, len, outbuf,
                sizeof(outbuf), &out_len) < 0)
        return -1;

    if (out_len > 0)
        tn5250_send_runtime_output(hhEmu, outbuf, out_len);

    tn5250_render(hhEmu);
    return 0;
    }

int emuTn5250StartPrinter(const HEMU hEmu)
    {
    HHEMU hhEmu;
    STTN5250 *pst;
    char message[256];

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    if (ut5250_runtime_start_printer(pst->runtime,
                "UltraTerminal5250Printer.exe", message,
                sizeof(message)) < 0)
        {
        tn5250_message(hhEmu, message);
        return -1;
        }
    tn5250_message(hhEmu, message);
    tn5250_render(hhEmu);
    return 0;
    }

int emuTn5250StopPrinter(const HEMU hEmu)
    {
    HHEMU hhEmu;
    STTN5250 *pst;
    char message[256];

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    ut5250_runtime_stop_printer(pst->runtime, message, sizeof(message));
    tn5250_message(hhEmu, message);
    tn5250_render(hhEmu);
    return 0;
    }

int emuTn5250ShowSettings(const HEMU hEmu)
    {
    HHEMU hhEmu;

    if (hEmu == 0)
        return -1;
    hhEmu = (HHEMU)hEmu;
    tn5250_message(hhEmu,
            "IBM 5250 settings are stored with the active UltraTerminal "
            "session: terminal type, device name, codepage, TLS, and printer "
            "options.");
    return 0;
    }

static void tn5250_notify(const HHEMU hhEmu, const int nEvent)
    {
    STTN5250 *pst;
    unsigned char outbuf[UT5250_TX_LIMIT];
    int out_len;

    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return;

    if (nEvent == EMU_EVENT_CONNECTED)
        {
        out_len = 0;
        if (ut5250_runtime_connected(pst->runtime, outbuf,
                    sizeof(outbuf), &out_len) == 0 && out_len > 0)
            tn5250_send_runtime_output(hhEmu, outbuf, out_len);
        tn5250_render(hhEmu);
        }
    }

static int tn5250_kbdin(const HHEMU hhEmu, int kcode, const int fTest)
    {
    STTN5250 *pst;
    int action;
    int ch;
    unsigned char outbuf[UT5250_TX_LIMIT];
    int out_len;

    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || pst->runtime == NULL)
        return -1;

    if (tn5250_key_action(kcode, &action, &ch) < 0)
        return -1;

    if (fTest)
        return 0;

    out_len = 0;
    if (ut5250_runtime_key(pst->runtime, action, ch, outbuf, sizeof(outbuf),
                &out_len) == 0)
        {
        if (out_len > 0)
            tn5250_send_runtime_output(hhEmu, outbuf, out_len);
        tn5250_render(hhEmu);
        }
    else
        {
        tn5250_render(hhEmu);
        }

    return -1;
    }

static void tn5250_render(const HHEMU hhEmu)
    {
    STTN5250 *pst;
    int row;
    int col;
    int addr;
    STATTR attr;
    const UT5250_CELL *cell;
    int ch;

    pst = (STTN5250 *)hhEmu->pvPrivate;
    if (pst == NULL || tn5250_query_snapshot(hhEmu, pst) < 0)
        return;

    hhEmu->emu_maxrow = pst->snapshot.rows - 1;
    hhEmu->emu_maxcol = pst->snapshot.cols - 1;
    hhEmu->emu_currow = pst->snapshot.cursor_row;
    hhEmu->emu_curcol = pst->snapshot.cursor_col;

    for (row = 0; row < MAX_EMUROWS; ++row)
        {
        for (col = 0; col < MAX_EMUCOLS; ++col)
            {
            if (row < pst->snapshot.rows && col < pst->snapshot.cols)
                {
                addr = row * pst->snapshot.cols + col;
                cell = &pst->snapshot.cells[addr];
                attr = tn5250_attr_for_cell(hhEmu, cell);
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

static int tn5250_send_runtime_output(const HHEMU hhEmu,
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

static int tn5250_key_action(int kcode, int *action, int *ch)
    {
    int vk;

    if (action == NULL || ch == NULL)
        return -1;

    *action = UT5250_KEY_NONE;
    *ch = 0;

    if (!(kcode & VIRTUAL_KEY))
        {
        if (kcode >= 0x20 && kcode <= 0x7e)
            {
            *action = UT5250_KEY_CHAR;
            *ch = kcode;
            return 0;
            }
        return -1;
        }

    vk = kcode & 0xffff;
    if (vk >= VK_F1 && vk <= VK_F12)
        {
        if (kcode & SHIFT_KEY)
            *action = UT5250_KEY_PF_BASE + (vk - VK_F1) + 13;
        else
            *action = UT5250_KEY_PF_BASE + (vk - VK_F1) + 1;
        return 0;
        }

    if (vk >= VK_F13 && vk <= VK_F24)
        {
        *action = UT5250_KEY_PF_BASE + (vk - VK_F13) + 13;
        return 0;
        }

    switch (vk)
        {
    case VK_RETURN:
        *action = UT5250_KEY_ENTER;
        break;
    case VK_ESCAPE:
        *action = UT5250_KEY_CLEAR;
        break;
    case VK_CANCEL:
        *action = UT5250_KEY_ATTENTION;
        break;
    case VK_SNAPSHOT:
        *action = (kcode & ALT_KEY) ? UT5250_KEY_SYSREQ : UT5250_KEY_PRINT;
        break;
    case VK_TAB:
        *action = (kcode & SHIFT_KEY) ? UT5250_KEY_BACKTAB : UT5250_KEY_TAB;
        break;
    case VK_LEFT:
        *action = (kcode & CTRL_KEY) ? UT5250_KEY_FIELD_EXIT :
                UT5250_KEY_LEFT;
        break;
    case VK_RIGHT:
        *action = (kcode & CTRL_KEY) ? UT5250_KEY_FIELD_PLUS :
                UT5250_KEY_RIGHT;
        break;
    case VK_UP:
        *action = UT5250_KEY_UP;
        break;
    case VK_DOWN:
        *action = UT5250_KEY_DOWN;
        break;
    case VK_PRIOR:
        *action = UT5250_KEY_ROLLDN;
        break;
    case VK_NEXT:
        *action = UT5250_KEY_ROLLUP;
        break;
    case VK_INSERT:
        *action = UT5250_KEY_INSERT;
        break;
    case VK_DELETE:
        *action = UT5250_KEY_DELETE;
        break;
    case VK_BACK:
        *action = UT5250_KEY_BACKSPACE;
        break;
    case VK_HOME:
        *action = UT5250_KEY_HOME;
        break;
    case VK_END:
        *action = UT5250_KEY_END;
        break;
    case VK_ADD:
        *action = UT5250_KEY_FIELD_PLUS;
        break;
    case VK_SUBTRACT:
        *action = UT5250_KEY_FIELD_MINUS;
        break;
    case VK_SCROLL:
        *action = UT5250_KEY_HELP;
        break;
    default:
        return -1;
        }

    return 0;
    }

static void tn5250_settings_from_emu(const HHEMU hhEmu,
        UT5250_SETTINGS *settings)
    {
    ut5250_settings_defaults(settings);
    settings->tls_mode = hhEmu->stUserSettings.nTn5250TlsMode;
    if (settings->tls_mode != UT5250_TLS_OFF &&
            settings->tls_mode != UT5250_TLS_DIRECT)
        settings->tls_mode = hhEmu->stUserSettings.fTn5250Tls ?
                UT5250_TLS_DIRECT : UT5250_TLS_OFF;
    settings->verify_tls = hhEmu->stUserSettings.fTn5250VerifyCert;
    settings->printer_enabled = hhEmu->stUserSettings.fTn5250Printer;
    tn5250_copy_wide(settings->terminal_type, sizeof(settings->terminal_type),
            hhEmu->stUserSettings.acTn5250TermType);
    tn5250_copy_wide(settings->device_name, sizeof(settings->device_name),
            hhEmu->stUserSettings.acTn5250DeviceName);
    tn5250_copy_wide(settings->host_codepage, sizeof(settings->host_codepage),
            hhEmu->stUserSettings.acTn5250HostCodePage);
    tn5250_copy_wide(settings->ca_path, sizeof(settings->ca_path),
            hhEmu->stUserSettings.acTn5250CaPath);
    tn5250_copy_wide(settings->cert_path, sizeof(settings->cert_path),
            hhEmu->stUserSettings.acTn5250CertPath);
    tn5250_copy_wide(settings->key_path, sizeof(settings->key_path),
            hhEmu->stUserSettings.acTn5250KeyPath);
    tn5250_copy_wide(settings->printer_device,
            sizeof(settings->printer_device),
            hhEmu->stUserSettings.acTn5250PrinterDevice);
    tn5250_copy_wide(settings->printer_options,
            sizeof(settings->printer_options),
            hhEmu->stUserSettings.acTn5250PrinterOptions);
    }

static void tn5250_copy_wide(char *dst, int dst_len, const WCHAR *src)
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

static STATTR tn5250_attr_for_cell(const HHEMU hhEmu,
        const UT5250_CELL *cell)
    {
    STATTR attr;

    memset(&attr, 0, sizeof(attr));
#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
    attr.txtclr = (unsigned int)(hhEmu->stUserSettings.nTextColor & 0x0f);
    attr.bkclr = (unsigned int)(hhEmu->stUserSettings.nBackgroundColor & 0x0f);
#else
    attr.txtclr = VC_WHITE;
    attr.bkclr = VC_BLACK;
#endif

    attr.txtclr = (unsigned int)(cell->foreground & 0x0f);
    attr.bkclr = (unsigned int)(cell->background & 0x0f);
    attr.protect = (cell->flags & UT5250_CELL_PROTECTED) ? 1 : 0;
    attr.blank = (cell->flags & UT5250_CELL_BLANK) ? 1 : 0;
    attr.revvid = (cell->flags & UT5250_CELL_REVERSE) ? 1 : 0;
    attr.undrln = (cell->flags & UT5250_CELL_UNDERLINE) ? 1 : 0;
    attr.blink = (cell->flags & UT5250_CELL_BLINK) ? 1 : 0;
    attr.hilite = (cell->foreground >= VC_GRAY) ? 1 : 0;

    if (cell->field_start)
        attr.blank = 1;

    return attr;
    }

static int tn5250_query_snapshot(const HHEMU hhEmu, STTN5250 *pst)
    {
    (void)hhEmu;
    if (pst == NULL || pst->runtime == NULL)
        return -1;
    return ut5250_runtime_snapshot(pst->runtime, &pst->snapshot);
    }

static void tn5250_message(const HHEMU hhEmu, const char *message)
    {
    if (message == NULL || message[0] == '\0')
        return;
    ut_message_box_utf8(sessQueryHwnd(hhEmu->hSession), message,
            L"UltraTerminal IBM 5250", MB_OK | MB_ICONINFORMATION);
    }

#endif
