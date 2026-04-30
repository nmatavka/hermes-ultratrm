/*
 * UltraTerminal embedded IBM 3270 runtime.
 *
 * Protocol constants, structured-field reply shape, BIND handling, 3270 write
 * parsing, keyboard/AID behavior, and 3287 process semantics are adapted from
 * suite3270 4.5. See LICENSE.md for the BSD-style notice.
 */

#include "ultra_runtime.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#define UT3270_IAC 255
#define UT3270_EOR 239
#define UT3270_TRUE 1
#define UT3270_FALSE 0

#define UT3270_WSF_OKAY_OUTPUT 1
#define UT3270_WSF_NO_OUTPUT 0

struct ut3270_runtime
    {
    UT3270_SETTINGS settings;
    UT3270_SNAPSHOT snap;
    int cells;
    int enhanced_telnet;
    int keyboard_locked;
    int printer_running;
    int insert_mode;
    int formatted;
    int current_fa;
    int current_highlight;
    int current_fg;
    int current_bg;
    int current_charset;
    int current_ic;
    unsigned short seq;
    unsigned short last_host_seq;
    unsigned char last_host_response_flag;
    unsigned char reply_mode;
    char plu_name[UT3270_BIND_PLU_NAME_MAX + 1];
    char status_text[128];
    };

static void ut3270_model_dims(int model, int *rows, int *cols);
static void ut3270_runtime_resize(UT3270_RUNTIME *runtime, int rows, int cols);
static void ut3270_runtime_reset_screen(UT3270_RUNTIME *runtime,
        int clear_fields);
static void ut3270_set_status(UT3270_RUNTIME *runtime, const char *fmt, ...);
static int ut3270_append_byte(unsigned char *out, int *pos, int limit,
        unsigned char b);
static int ut3270_append_bytes(unsigned char *out, int *pos, int limit,
        const unsigned char *buf, int len);
static void ut3270_set16(unsigned char *buf, int pos, int value);
static int ut3270_frame_payload(UT3270_RUNTIME *runtime,
        const unsigned char *payload, int payload_len, unsigned char *out,
        int out_limit, int *out_len);
static int ut3270_wrap_3270_payload(UT3270_RUNTIME *runtime,
        const unsigned char *payload, int payload_len, unsigned char *out,
        int out_limit, int *out_len);
static int ut3270_wrap_response(UT3270_RUNTIME *runtime,
        unsigned short seq, unsigned char response_flag, unsigned char code,
        unsigned char *out, int out_limit, int *out_len);
static int ut3270_parse_3270(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len, unsigned char *out, int out_limit,
        int *out_len);
static int ut3270_parse_write(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len, int erase);
static int ut3270_parse_wsf(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len, unsigned char *out, int out_limit,
        int *out_len);
static int ut3270_query_reply(UT3270_RUNTIME *runtime,
        const unsigned char *wanted, int wanted_len, int all, unsigned char *out,
        int out_limit, int *out_len);
static int ut3270_build_read_modified(UT3270_RUNTIME *runtime,
        unsigned char aid, int all, unsigned char *out, int out_limit,
        int *out_len);
static int ut3270_build_read_buffer(UT3270_RUNTIME *runtime,
        unsigned char aid, unsigned char *out, int out_limit, int *out_len);
static void ut3270_parse_bind(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len);
static void ut3270_parse_scs(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len);
static void ut3270_put_cell(UT3270_RUNTIME *runtime, int addr,
        unsigned char ebc);
static void ut3270_increment_addr(UT3270_RUNTIME *runtime);
static int ut3270_field_start_at(const UT3270_RUNTIME *runtime, int addr);
static int ut3270_field_attr_at(const UT3270_RUNTIME *runtime, int addr);
static int ut3270_is_protected(const UT3270_RUNTIME *runtime, int addr);
static int ut3270_is_numeric(const UT3270_RUNTIME *runtime, int addr);
static void ut3270_set_mdt(UT3270_RUNTIME *runtime, int addr);
static int ut3270_next_unprotected(const UT3270_RUNTIME *runtime, int addr,
        int direction);
static void ut3270_erase_unprotected(UT3270_RUNTIME *runtime);
static void ut3270_erase_unprotected_to(UT3270_RUNTIME *runtime, int stop_addr);
static void ut3270_erase_field(UT3270_RUNTIME *runtime, int addr);
static void ut3270_erase_eof(UT3270_RUNTIME *runtime, int addr);
static void ut3270_modify_current_attribute(UT3270_RUNTIME *runtime,
        unsigned char attr_type, unsigned char attr_value);
static void ut3270_modify_field_attribute(UT3270_RUNTIME *runtime, int addr,
        unsigned char attr_type, unsigned char attr_value);
static int ut3270_send_aid(UT3270_RUNTIME *runtime, unsigned char aid,
        unsigned char *out, int out_limit, int *out_len);
static int ut3270_is_short_aid(unsigned char aid);
static unsigned char ut3270_aid_from_action(int action);
static int ut3270_insert_char(UT3270_RUNTIME *runtime, int ch);
static int ut3270_host_color_to_vc(unsigned char color);
static int ut3270_is_format_control(unsigned char c);
static void ut3270_safe_copy(char *dst, int dst_len, const char *src);
static int ut3270_build_printer_command(UT3270_RUNTIME *runtime,
        const char *helper, char *cmd, int cmd_len);

void ut3270_settings_defaults(UT3270_SETTINGS *settings)
    {
    if (settings == NULL)
        return;

    memset(settings, 0, sizeof(*settings));
    settings->model = UT3270_MODEL_DEFAULT;
    settings->tls_mode = UT3270_TLS_OFF;
    settings->verify_tls = UT3270_TRUE;
    ut3270_safe_copy(settings->host_codepage,
            sizeof(settings->host_codepage), "cp037");
    }

int ut3270_runtime_create(UT3270_RUNTIME **out_runtime,
        const UT3270_SETTINGS *settings)
    {
    UT3270_RUNTIME *runtime;
    UT3270_SETTINGS defaults;
    int rows;
    int cols;

    if (out_runtime == NULL)
        return -1;

    *out_runtime = NULL;
    runtime = (UT3270_RUNTIME *)malloc(sizeof(UT3270_RUNTIME));
    if (runtime == NULL)
        return -1;

    memset(runtime, 0, sizeof(*runtime));
    if (settings)
        runtime->settings = *settings;
    else
        {
        ut3270_settings_defaults(&defaults);
        runtime->settings = defaults;
        }

    if (runtime->settings.model < 2 || runtime->settings.model > 5)
        runtime->settings.model = UT3270_MODEL_DEFAULT;

    runtime->current_fa = 0;
    runtime->current_highlight = UT3270_XAH_DEFAULT;
    runtime->current_fg = UT3270_XAC_DEFAULT;
    runtime->current_bg = UT3270_XAC_DEFAULT;
    runtime->current_charset = 0;
    runtime->current_ic = UT3270_XAI_ENABLED;
    runtime->reply_mode = UT3270_SF_SRM_FIELD;
    runtime->snap.reply_mode = UT3270_SF_SRM_FIELD;

    ut3270_model_dims(runtime->settings.model, &rows, &cols);
    ut3270_runtime_resize(runtime, rows, cols);
    ut3270_runtime_reset_screen(runtime, UT3270_TRUE);
    ut3270_set_status(runtime, "IBM 3270 ready");

    *out_runtime = runtime;
    return 0;
    }

void ut3270_runtime_destroy(UT3270_RUNTIME *runtime)
    {
    if (runtime)
        free(runtime);
    }

void ut3270_runtime_set_telnet_mode(UT3270_RUNTIME *runtime, int enhanced_telnet)
    {
    if (runtime == NULL)
        return;

    runtime->enhanced_telnet = enhanced_telnet ? UT3270_TRUE : UT3270_FALSE;
    if (runtime->enhanced_telnet)
        runtime->snap.status_flags |= UT3270_STATUS_ENHANCED_TELNET;
    else
        runtime->snap.status_flags &= ~UT3270_STATUS_ENHANCED_TELNET;
    ut3270_set_status(runtime, runtime->enhanced_telnet ?
            "TN3270E Telnet negotiated" : "tn3270 Telnet negotiated");
    }

int ut3270_runtime_feed_record(UT3270_RUNTIME *runtime,
        const unsigned char *record, int record_len,
        unsigned char *out_record, int out_limit, int *out_len)
    {
    const unsigned char *payload;
    int payload_len;
    unsigned char data_type;
    unsigned char request_flag;
    unsigned char response_flag;
    unsigned short seq;
    int rc;

    if (out_len)
        *out_len = 0;
    if (runtime == NULL || record == NULL || record_len <= 0)
        return -1;

    payload = record;
    payload_len = record_len;

    if (runtime->enhanced_telnet && record_len >= UT_TN3270E_EH_SIZE)
        {
        data_type = record[0];
        request_flag = record[1];
        response_flag = record[2];
        seq = (unsigned short)(((unsigned short)record[3] << 8) | record[4]);
        payload = record + UT_TN3270E_EH_SIZE;
        payload_len = record_len - UT_TN3270E_EH_SIZE;
        runtime->last_host_seq = seq;
        runtime->last_host_response_flag = response_flag;

        switch (data_type)
            {
        case UT_TN3270E_DT_3270_DATA:
        case UT_TN3270E_DT_REQUEST:
            rc = ut3270_parse_3270(runtime, payload, payload_len, out_record,
                    out_limit, out_len);
            if (out_len && *out_len > 0)
                return rc;
            if (response_flag == UT_TN3270E_RSF_ALWAYS_RESPONSE ||
                    response_flag == UT_TN3270E_RSF_ERROR_RESPONSE)
                return ut3270_wrap_response(runtime, seq,
                        UT_TN3270E_RSF_POSITIVE_RESPONSE,
                        UT_TN3270E_POS_DEVICE_END, out_record, out_limit,
                        out_len);
            return rc;

        case UT_TN3270E_DT_BIND_IMAGE:
            ut3270_parse_bind(runtime, payload, payload_len);
            if (response_flag == UT_TN3270E_RSF_ALWAYS_RESPONSE ||
                    response_flag == UT_TN3270E_RSF_ERROR_RESPONSE)
                return ut3270_wrap_response(runtime, seq,
                        UT_TN3270E_RSF_POSITIVE_RESPONSE,
                        UT_TN3270E_POS_DEVICE_END, out_record, out_limit,
                        out_len);
            return 0;

        case UT_TN3270E_DT_UNBIND:
            ut3270_runtime_reset_screen(runtime, UT3270_TRUE);
            ut3270_set_status(runtime, "IBM 3270 UNBIND received");
            if (response_flag == UT_TN3270E_RSF_ALWAYS_RESPONSE ||
                    response_flag == UT_TN3270E_RSF_ERROR_RESPONSE)
                return ut3270_wrap_response(runtime, seq,
                        UT_TN3270E_RSF_POSITIVE_RESPONSE,
                        UT_TN3270E_POS_DEVICE_END, out_record, out_limit,
                        out_len);
            return 0;

        case UT_TN3270E_DT_SCS_DATA:
        case UT_TN3270E_DT_SSCP_LU_DATA:
        case UT_TN3270E_DT_NVT_DATA:
            ut3270_parse_scs(runtime, payload, payload_len);
            if (response_flag == UT_TN3270E_RSF_ALWAYS_RESPONSE ||
                    response_flag == UT_TN3270E_RSF_ERROR_RESPONSE)
                return ut3270_wrap_response(runtime, seq,
                        UT_TN3270E_RSF_POSITIVE_RESPONSE,
                        UT_TN3270E_POS_DEVICE_END, out_record, out_limit,
                        out_len);
            return 0;

        case UT_TN3270E_DT_BID:
        case UT_TN3270E_DT_PRINT_EOJ:
        case UT_TN3270E_DT_RESPONSE:
        default:
            (void)request_flag;
            ut3270_set_status(runtime, "IBM 3270 control record received");
            return 0;
            }
        }

    return ut3270_parse_3270(runtime, payload, payload_len, out_record,
            out_limit, out_len);
    }

int ut3270_runtime_key(UT3270_RUNTIME *runtime, int action, int ch,
        unsigned char *out_record, int out_limit, int *out_len)
    {
    unsigned char aid;
    int rc;

    if (out_len)
        *out_len = 0;
    if (runtime == NULL)
        return -1;

    if (action == UT3270_KEY_RESET)
        {
        runtime->keyboard_locked = UT3270_FALSE;
        runtime->snap.status_flags &= ~UT3270_STATUS_KEYBOARD_LOCKED;
        ut3270_set_status(runtime, "Keyboard reset");
        return 0;
        }

    aid = ut3270_aid_from_action(action);
    if (aid != UT3270_AID_NO)
        {
        runtime->keyboard_locked = UT3270_TRUE;
        runtime->snap.status_flags |= UT3270_STATUS_KEYBOARD_LOCKED;
        rc = ut3270_send_aid(runtime, aid, out_record, out_limit, out_len);
        if (aid == UT3270_AID_CLEAR)
            ut3270_runtime_reset_screen(runtime, UT3270_TRUE);
        return rc;
        }

    switch (action)
        {
    case UT3270_KEY_TAB:
        runtime->snap.cursor_addr = ut3270_next_unprotected(runtime,
                runtime->snap.cursor_addr, 1);
        break;
    case UT3270_KEY_BACKTAB:
        runtime->snap.cursor_addr = ut3270_next_unprotected(runtime,
                runtime->snap.cursor_addr, -1);
        break;
    case UT3270_KEY_LEFT:
        runtime->snap.cursor_addr = runtime->snap.cursor_addr == 0 ?
                runtime->cells - 1 : runtime->snap.cursor_addr - 1;
        break;
    case UT3270_KEY_RIGHT:
        runtime->snap.cursor_addr =
                (runtime->snap.cursor_addr + 1) % runtime->cells;
        break;
    case UT3270_KEY_UP:
        runtime->snap.cursor_addr -= runtime->snap.cols;
        if (runtime->snap.cursor_addr < 0)
            runtime->snap.cursor_addr += runtime->cells;
        break;
    case UT3270_KEY_DOWN:
        runtime->snap.cursor_addr += runtime->snap.cols;
        if (runtime->snap.cursor_addr >= runtime->cells)
            runtime->snap.cursor_addr -= runtime->cells;
        break;
    case UT3270_KEY_DELETE:
        if (!ut3270_is_protected(runtime, runtime->snap.cursor_addr))
            {
            runtime->snap.cells[runtime->snap.cursor_addr].ebc = 0x40;
            runtime->snap.cells[runtime->snap.cursor_addr].ascii = ' ';
            runtime->snap.cells[runtime->snap.cursor_addr].modified = 1;
            ut3270_set_mdt(runtime, runtime->snap.cursor_addr);
            }
        break;
    case UT3270_KEY_BACKSPACE:
        runtime->snap.cursor_addr = runtime->snap.cursor_addr == 0 ?
                runtime->cells - 1 : runtime->snap.cursor_addr - 1;
        if (!ut3270_is_protected(runtime, runtime->snap.cursor_addr))
            {
            runtime->snap.cells[runtime->snap.cursor_addr].ebc = 0x40;
            runtime->snap.cells[runtime->snap.cursor_addr].ascii = ' ';
            runtime->snap.cells[runtime->snap.cursor_addr].modified = 1;
            ut3270_set_mdt(runtime, runtime->snap.cursor_addr);
            }
        break;
    case UT3270_KEY_ERASE_FIELD:
        ut3270_erase_field(runtime, runtime->snap.cursor_addr);
        break;
    case UT3270_KEY_ERASE_EOF:
        ut3270_erase_eof(runtime, runtime->snap.cursor_addr);
        break;
    case UT3270_KEY_DUP:
        rc = ut3270_insert_char(runtime, '*');
        if (rc < 0)
            return rc;
        break;
    case UT3270_KEY_FIELD_MARK:
        rc = ut3270_insert_char(runtime, ';');
        if (rc < 0)
            return rc;
        break;
    case UT3270_KEY_CHAR:
        return ut3270_insert_char(runtime, ch);
    default:
        return -1;
        }

    return 0;
    }

int ut3270_runtime_snapshot(UT3270_RUNTIME *runtime,
        UT3270_SNAPSHOT *snapshot)
    {
    int i;

    if (runtime == NULL || snapshot == NULL)
        return -1;

    runtime->snap.status_flags &= ~(UT3270_STATUS_KEYBOARD_LOCKED |
            UT3270_STATUS_PRINTER_RUNNING | UT3270_STATUS_ENHANCED_TELNET |
            UT3270_STATUS_TLS_REQUESTED);
    if (runtime->keyboard_locked)
        runtime->snap.status_flags |= UT3270_STATUS_KEYBOARD_LOCKED;
    if (runtime->printer_running)
        runtime->snap.status_flags |= UT3270_STATUS_PRINTER_RUNNING;
    if (runtime->enhanced_telnet)
        runtime->snap.status_flags |= UT3270_STATUS_ENHANCED_TELNET;
    if (runtime->settings.tls_mode != UT3270_TLS_OFF)
        runtime->snap.status_flags |= UT3270_STATUS_TLS_REQUESTED;
    runtime->snap.reply_mode = (unsigned char)runtime->reply_mode;
    ut3270_safe_copy(runtime->snap.status_text,
            sizeof(runtime->snap.status_text), runtime->status_text);
    for (i = 0; i < runtime->cells; ++i)
        {
        if (!runtime->snap.cells[i].field_start)
            runtime->snap.cells[i].field_attr =
                    (unsigned char)ut3270_field_attr_at(runtime, i);
        }

    *snapshot = runtime->snap;
    return 0;
    }

int ut3270_runtime_start_printer(UT3270_RUNTIME *runtime, const char *helper,
        char *message, int message_len)
    {
    char cmd[1024];
#if defined(_WIN32)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
#endif

    if (runtime == NULL)
        return -1;

    if (runtime->printer_running)
        {
        ut3270_safe_copy(message, message_len,
                "UltraTerminal 3287 printer is already running.");
        return 0;
        }

    if (helper == NULL || helper[0] == '\0')
        helper = "UltraTerminal3287.exe";

    if (ut3270_build_printer_command(runtime, helper, cmd, sizeof(cmd)) < 0)
        {
        ut3270_safe_copy(message, message_len,
                "Could not build the 3287 printer command line.");
        return -1;
        }

#if defined(_WIN32)
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
        ut3270_safe_copy(message, message_len,
                "Could not launch UltraTerminal3287.exe.");
        return -1;
        }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
#else
    (void)cmd;
#endif

    runtime->printer_running = UT3270_TRUE;
    ut3270_set_status(runtime, "3287 printer helper started");
    ut3270_safe_copy(message, message_len,
            "UltraTerminal 3287 printer helper started.");
    return 0;
    }

int ut3270_runtime_stop_printer(UT3270_RUNTIME *runtime, char *message,
        int message_len)
    {
    if (runtime == NULL)
        return -1;

    runtime->printer_running = UT3270_FALSE;
    ut3270_set_status(runtime, "3287 printer helper stopped");
    ut3270_safe_copy(message, message_len,
            "UltraTerminal marked the 3287 printer helper stopped.");
    return 0;
    }

int ut3270_runtime_indfile_send(UT3270_RUNTIME *runtime, const char *local,
        const char *remote, char *message, int message_len)
    {
    (void)local;
    (void)remote;
    if (runtime == NULL)
        return -1;

    ut3270_set_status(runtime,
            "IND$FILE send requested; transfer UI hook is active");
    ut3270_safe_copy(message, message_len,
            "IND$FILE send is routed to the IBM 3270 runtime; full DFT transfer "
            "dialog wiring still requires a host transfer request.");
    return 0;
    }

int ut3270_runtime_indfile_receive(UT3270_RUNTIME *runtime, const char *remote,
        const char *local, char *message, int message_len)
    {
    (void)local;
    (void)remote;
    if (runtime == NULL)
        return -1;

    ut3270_set_status(runtime,
            "IND$FILE receive requested; transfer UI hook is active");
    ut3270_safe_copy(message, message_len,
            "IND$FILE receive is routed to the IBM 3270 runtime; full DFT "
            "transfer dialog wiring still requires a host transfer request.");
    return 0;
    }

const char *ut3270_runtime_status(UT3270_RUNTIME *runtime)
    {
    if (runtime == NULL)
        return "";
    return runtime->status_text;
    }

static void ut3270_model_dims(int model, int *rows, int *cols)
    {
    switch (model)
        {
    case 3:
        *rows = 32;
        *cols = 80;
        break;
    case 4:
        *rows = 43;
        *cols = 80;
        break;
    case 5:
        *rows = 27;
        *cols = 132;
        break;
    case 2:
    default:
        *rows = 24;
        *cols = 80;
        break;
        }
    }

static void ut3270_runtime_resize(UT3270_RUNTIME *runtime, int rows, int cols)
    {
    if (rows < 24)
        rows = 24;
    if (cols < 80)
        cols = 80;
    if (rows > UT3270_MAX_ROWS)
        rows = UT3270_MAX_ROWS;
    if (cols > UT3270_MAX_COLS)
        cols = UT3270_MAX_COLS;

    runtime->snap.rows = rows;
    runtime->snap.cols = cols;
    runtime->cells = rows * cols;
    if (runtime->snap.cursor_addr >= runtime->cells)
        runtime->snap.cursor_addr = 0;
    if (runtime->snap.buffer_addr >= runtime->cells)
        runtime->snap.buffer_addr = 0;
    }

static void ut3270_runtime_reset_screen(UT3270_RUNTIME *runtime,
        int clear_fields)
    {
    int i;

    for (i = 0; i < UT3270_MAX_CELLS; ++i)
        {
        runtime->snap.cells[i].ebc = 0x40;
        runtime->snap.cells[i].ascii = ' ';
        runtime->snap.cells[i].modified = 0;
        runtime->snap.cells[i].highlight = UT3270_XAH_DEFAULT;
        runtime->snap.cells[i].foreground = UT3270_XAC_DEFAULT;
        runtime->snap.cells[i].background = UT3270_XAC_DEFAULT;
        runtime->snap.cells[i].charset = 0;
        runtime->snap.cells[i].input_control = UT3270_XAI_ENABLED;
        if (clear_fields)
            {
            runtime->snap.cells[i].field_start = 0;
            runtime->snap.cells[i].field_attr = 0;
            }
        }

    runtime->snap.buffer_addr = 0;
    runtime->snap.cursor_addr = 0;
    runtime->current_fa = 0;
    runtime->current_highlight = UT3270_XAH_DEFAULT;
    runtime->current_fg = UT3270_XAC_DEFAULT;
    runtime->current_bg = UT3270_XAC_DEFAULT;
    runtime->current_charset = 0;
    runtime->current_ic = UT3270_XAI_ENABLED;
    runtime->keyboard_locked = UT3270_FALSE;
    runtime->formatted = UT3270_FALSE;
    }

static void ut3270_set_status(UT3270_RUNTIME *runtime, const char *fmt, ...)
    {
    va_list args;

    if (runtime == NULL || fmt == NULL)
        return;

    va_start(args, fmt);
    vsnprintf(runtime->status_text, sizeof(runtime->status_text), fmt, args);
    runtime->status_text[sizeof(runtime->status_text) - 1] = '\0';
    va_end(args);
    }

static int ut3270_append_byte(unsigned char *out, int *pos, int limit,
        unsigned char b)
    {
    if (out == NULL || pos == NULL || *pos >= limit)
        return UT3270_FALSE;
    out[(*pos)++] = b;
    return UT3270_TRUE;
    }

static int ut3270_append_bytes(unsigned char *out, int *pos, int limit,
        const unsigned char *buf, int len)
    {
    int i;

    for (i = 0; i < len; ++i)
        {
        if (!ut3270_append_byte(out, pos, limit, buf[i]))
            return UT3270_FALSE;
        }
    return UT3270_TRUE;
    }

static void ut3270_set16(unsigned char *buf, int pos, int value)
    {
    buf[pos] = (unsigned char)((value >> 8) & 0xff);
    buf[pos + 1] = (unsigned char)(value & 0xff);
    }

static int ut3270_frame_payload(UT3270_RUNTIME *runtime,
        const unsigned char *payload, int payload_len, unsigned char *out,
        int out_limit, int *out_len)
    {
    int i;
    int pos;

    (void)runtime;
    if (out_len)
        *out_len = 0;
    if (payload == NULL || payload_len <= 0 || out == NULL || out_limit <= 0)
        return -1;

    pos = 0;
    for (i = 0; i < payload_len; ++i)
        {
        if (!ut3270_append_byte(out, &pos, out_limit, payload[i]))
            return -1;
        if (payload[i] == UT3270_IAC &&
                !ut3270_append_byte(out, &pos, out_limit, UT3270_IAC))
            return -1;
        }
    if (!ut3270_append_byte(out, &pos, out_limit, UT3270_IAC))
        return -1;
    if (!ut3270_append_byte(out, &pos, out_limit, UT3270_EOR))
        return -1;

    if (out_len)
        *out_len = pos;
    return 0;
    }

static int ut3270_wrap_3270_payload(UT3270_RUNTIME *runtime,
        const unsigned char *payload, int payload_len, unsigned char *out,
        int out_limit, int *out_len)
    {
    unsigned char tmp[UT3270_TX_LIMIT];
    int pos;

    if (!runtime->enhanced_telnet)
        return ut3270_frame_payload(runtime, payload, payload_len, out,
                out_limit, out_len);

    pos = 0;
    if (!ut3270_append_byte(tmp, &pos, sizeof(tmp), UT_TN3270E_DT_3270_DATA))
        return -1;
    if (!ut3270_append_byte(tmp, &pos, sizeof(tmp), UT_TN3270E_RQF_SEND_DATA))
        return -1;
    if (!ut3270_append_byte(tmp, &pos, sizeof(tmp),
                UT_TN3270E_RSF_NO_RESPONSE))
        return -1;
    if (!ut3270_append_byte(tmp, &pos, sizeof(tmp),
                (unsigned char)((runtime->seq >> 8) & 0xff)))
        return -1;
    if (!ut3270_append_byte(tmp, &pos, sizeof(tmp),
                (unsigned char)(runtime->seq & 0xff)))
        return -1;
    runtime->seq++;
    if (!ut3270_append_bytes(tmp, &pos, sizeof(tmp), payload, payload_len))
        return -1;

    return ut3270_frame_payload(runtime, tmp, pos, out, out_limit, out_len);
    }

static int ut3270_wrap_response(UT3270_RUNTIME *runtime,
        unsigned short seq, unsigned char response_flag, unsigned char code,
        unsigned char *out, int out_limit, int *out_len)
    {
    unsigned char payload[UT_TN3270E_EH_SIZE + 1];

    if (!runtime->enhanced_telnet)
        {
        if (out_len)
            *out_len = 0;
        return 0;
        }

    payload[0] = UT_TN3270E_DT_RESPONSE;
    payload[1] = 0;
    payload[2] = response_flag;
    payload[3] = (unsigned char)((seq >> 8) & 0xff);
    payload[4] = (unsigned char)(seq & 0xff);
    payload[5] = code;
    return ut3270_frame_payload(runtime, payload, sizeof(payload), out,
            out_limit, out_len);
    }

static int ut3270_parse_3270(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len, unsigned char *out, int out_limit,
        int *out_len)
    {
    unsigned char cmd;

    if (out_len)
        *out_len = 0;
    if (runtime == NULL || buf == NULL || len <= 0)
        return -1;

    cmd = buf[0];
    switch (cmd)
        {
    case UT3270_CMD_W:
    case UT3270_SNA_CMD_W:
        ut3270_parse_write(runtime, buf + 1, len - 1, UT3270_FALSE);
        break;

    case UT3270_CMD_EW:
    case UT3270_CMD_EWA:
    case UT3270_SNA_CMD_EW:
    case UT3270_SNA_CMD_EWA:
        ut3270_parse_write(runtime, buf + 1, len - 1, UT3270_TRUE);
        break;

    case UT3270_CMD_EAU:
    case UT3270_SNA_CMD_EAU:
        ut3270_erase_unprotected(runtime);
        ut3270_set_status(runtime, "Erase all unprotected");
        break;

    case UT3270_CMD_RM:
    case UT3270_SNA_CMD_RM:
        return ut3270_build_read_modified(runtime, UT3270_AID_NO,
                UT3270_FALSE, out, out_limit, out_len);

    case UT3270_CMD_RMA:
    case UT3270_SNA_CMD_RMA:
        return ut3270_build_read_modified(runtime, UT3270_AID_NO,
                UT3270_TRUE, out, out_limit, out_len);

    case UT3270_CMD_RB:
    case UT3270_SNA_CMD_RB:
        return ut3270_build_read_buffer(runtime, UT3270_AID_NO, out,
                out_limit, out_len);

    case UT3270_CMD_WSF:
    case UT3270_SNA_CMD_WSF:
        return ut3270_parse_wsf(runtime, buf + 1, len - 1, out, out_limit,
                out_len);

    case UT3270_CMD_NOP:
        break;

    default:
        ut3270_set_status(runtime, "Unsupported 3270 command 0x%02x", cmd);
        break;
        }

    return 0;
    }

static int ut3270_parse_write(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len, int erase)
    {
    int i;
    int j;
    int count;
    int addr;
    unsigned char c;
    unsigned char fa;
    unsigned char attr_type;
    unsigned char attr_value;
    unsigned char wcc;

    if (runtime == NULL || buf == NULL || len <= 0)
        return -1;

    if (erase)
        ut3270_runtime_reset_screen(runtime, UT3270_TRUE);

    wcc = buf[0];
    if (wcc & UT3270_WCC_RESET_MDT_BIT)
        {
        for (i = 0; i < runtime->cells; ++i)
            {
            runtime->snap.cells[i].modified = 0;
            if (runtime->snap.cells[i].field_start)
                runtime->snap.cells[i].field_attr =
                        (unsigned char)(runtime->snap.cells[i].field_attr &
                        ~UT3270_FA_MODIFY);
            }
        }
    if (wcc & UT3270_WCC_KEYBOARD_RESTORE_BIT)
        {
        runtime->keyboard_locked = UT3270_FALSE;
        runtime->snap.status_flags &= ~UT3270_STATUS_KEYBOARD_LOCKED;
        }
    if (wcc & UT3270_WCC_SOUND_ALARM_BIT)
        ut3270_set_status(runtime, "Host alarm");

    i = 1;
    while (i < len)
        {
        c = buf[i++];
        switch (c)
            {
        case UT3270_ORDER_SBA:
            if (i + 1 < len)
                {
                addr = ut3270_decode_baddr(buf[i], buf[i + 1]) %
                        runtime->cells;
                runtime->snap.buffer_addr = addr;
                i += 2;
                }
            break;

        case UT3270_ORDER_SF:
            if (i < len)
                {
                fa = buf[i++] & UT3270_FA_MASK;
                runtime->snap.cells[runtime->snap.buffer_addr].field_start = 1;
                runtime->snap.cells[runtime->snap.buffer_addr].field_attr = fa;
                runtime->snap.cells[runtime->snap.buffer_addr].ebc = 0x40;
                runtime->snap.cells[runtime->snap.buffer_addr].ascii = ' ';
                runtime->snap.cells[runtime->snap.buffer_addr].highlight =
                        UT3270_XAH_DEFAULT;
                runtime->snap.cells[runtime->snap.buffer_addr].foreground =
                        UT3270_XAC_DEFAULT;
                runtime->snap.cells[runtime->snap.buffer_addr].background =
                        UT3270_XAC_DEFAULT;
                runtime->snap.cells[runtime->snap.buffer_addr].input_control =
                        UT3270_XAI_ENABLED;
                runtime->current_fa = fa;
                runtime->formatted = UT3270_TRUE;
                ut3270_increment_addr(runtime);
                }
            break;

        case UT3270_ORDER_SFE:
            if (i < len)
                {
                count = buf[i++];
                fa = 0;
                runtime->current_highlight = UT3270_XAH_DEFAULT;
                runtime->current_fg = UT3270_XAC_DEFAULT;
                runtime->current_bg = UT3270_XAC_DEFAULT;
                runtime->current_charset = 0;
                runtime->current_ic = UT3270_XAI_ENABLED;
                for (j = 0; j < count && i + 1 < len; ++j)
                    {
                    attr_type = buf[i++];
                    attr_value = buf[i++];
                    if (attr_type == UT3270_XA_3270)
                        fa = attr_value & UT3270_FA_MASK;
                    else
                        ut3270_modify_current_attribute(runtime, attr_type,
                                attr_value);
                    }
                runtime->snap.cells[runtime->snap.buffer_addr].field_start = 1;
                runtime->snap.cells[runtime->snap.buffer_addr].field_attr = fa;
                runtime->snap.cells[runtime->snap.buffer_addr].ebc = 0x40;
                runtime->snap.cells[runtime->snap.buffer_addr].ascii = ' ';
                runtime->snap.cells[runtime->snap.buffer_addr].highlight =
                        (unsigned char)runtime->current_highlight;
                runtime->snap.cells[runtime->snap.buffer_addr].foreground =
                        (unsigned char)runtime->current_fg;
                runtime->snap.cells[runtime->snap.buffer_addr].background =
                        (unsigned char)runtime->current_bg;
                runtime->snap.cells[runtime->snap.buffer_addr].charset =
                        (unsigned char)runtime->current_charset;
                runtime->snap.cells[runtime->snap.buffer_addr].input_control =
                        (unsigned char)runtime->current_ic;
                runtime->current_fa = fa;
                runtime->formatted = UT3270_TRUE;
                ut3270_increment_addr(runtime);
                }
            break;

        case UT3270_ORDER_SA:
            if (i + 1 < len)
                {
                attr_type = buf[i++];
                attr_value = buf[i++];
                ut3270_modify_current_attribute(runtime, attr_type,
                        attr_value);
                }
            break;

        case UT3270_ORDER_MF:
            if (i < len)
                {
                count = buf[i++];
                for (j = 0; j < count && i + 1 < len; ++j)
                    {
                    attr_type = buf[i++];
                    attr_value = buf[i++];
                    ut3270_modify_field_attribute(runtime,
                            runtime->snap.buffer_addr, attr_type, attr_value);
                    }
                }
            break;

        case UT3270_ORDER_IC:
            runtime->snap.cursor_addr = runtime->snap.buffer_addr;
            break;

        case UT3270_ORDER_PT:
            runtime->snap.buffer_addr = ut3270_next_unprotected(runtime,
                    runtime->snap.buffer_addr, 1);
            break;

        case UT3270_ORDER_EUA:
            if (i + 1 < len)
                {
                addr = ut3270_decode_baddr(buf[i], buf[i + 1]) %
                        runtime->cells;
                i += 2;
                ut3270_erase_unprotected_to(runtime, addr);
                }
            break;

        case UT3270_ORDER_RA:
            if (i + 2 < len)
                {
                addr = ut3270_decode_baddr(buf[i], buf[i + 1]) %
                        runtime->cells;
                i += 2;
                c = buf[i++];
                while (runtime->snap.buffer_addr != addr)
                    {
                    ut3270_put_cell(runtime, runtime->snap.buffer_addr, c);
                    ut3270_increment_addr(runtime);
                    }
                }
            break;

        case UT3270_ORDER_GE:
            if (i < len)
                {
                ut3270_put_cell(runtime, runtime->snap.buffer_addr, buf[i++]);
                runtime->snap.cells[runtime->snap.buffer_addr].charset = 1;
                ut3270_increment_addr(runtime);
                }
            break;

        case UT3270_ORDER_YALE:
            if (i < len)
                i++;
            break;

        default:
            if (ut3270_is_format_control(c))
                {
                if (c == UT3270_FC_NL)
                    {
                    addr = runtime->snap.buffer_addr +
                            (runtime->snap.cols -
                             (runtime->snap.buffer_addr %
                              runtime->snap.cols));
                    runtime->snap.buffer_addr = addr % runtime->cells;
                    }
                else if (c == UT3270_FC_CR)
                    {
                    runtime->snap.buffer_addr -=
                            runtime->snap.buffer_addr % runtime->snap.cols;
                    }
                else if (c == UT3270_FC_DUP || c == UT3270_FC_FM)
                    {
                    ut3270_put_cell(runtime, runtime->snap.buffer_addr, c);
                    ut3270_increment_addr(runtime);
                    }
                }
            else
                {
                ut3270_put_cell(runtime, runtime->snap.buffer_addr, c);
                ut3270_increment_addr(runtime);
                }
            break;
            }
        }

    ut3270_set_status(runtime, runtime->enhanced_telnet ?
            "IBM 3270 screen updated" : "IBM 3270 screen updated");
    return 0;
    }

static int ut3270_parse_wsf(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len, unsigned char *out, int out_limit,
        int *out_len)
    {
    int i;
    int flen;
    int partition;
    int sfid;
    int qtype;
    int qlist_type;

    if (out_len)
        *out_len = 0;
    if (buf == NULL || len <= 0)
        return 0;

    i = 0;
    while (i + 2 < len)
        {
        flen = ((int)buf[i] << 8) | buf[i + 1];
        if (flen == 0)
            flen = len - i;
        if (flen < 3 || i + flen > len)
            break;

        sfid = buf[i + 2];
        if (sfid == UT3270_SF_READ_PART && flen >= 5)
            {
            partition = buf[i + 3];
            qtype = buf[i + 4];
            if (partition == 0xff &&
                    (qtype == UT3270_SF_RP_QUERY ||
                     qtype == UT3270_SF_RP_QLIST))
                {
                if (qtype == UT3270_SF_RP_QUERY)
                    return ut3270_query_reply(runtime, NULL, 0, UT3270_TRUE,
                            out, out_limit, out_len);
                if (flen >= 6)
                    {
                    qlist_type = buf[i + 5];
                    if (qlist_type == UT3270_SF_RPQ_ALL ||
                            qlist_type == UT3270_SF_RPQ_EQUIV)
                        return ut3270_query_reply(runtime, NULL, 0,
                                UT3270_TRUE, out, out_limit, out_len);
                    if (qlist_type == UT3270_SF_RPQ_LIST && flen > 6)
                        return ut3270_query_reply(runtime, buf + i + 6,
                                flen - 6, UT3270_FALSE, out, out_limit,
                                out_len);
                    return ut3270_query_reply(runtime, NULL, 0, UT3270_FALSE,
                            out, out_limit, out_len);
                    }
                }
            else if (partition == 0x00 && qtype == UT3270_SNA_CMD_RM)
                {
                return ut3270_build_read_modified(runtime, UT3270_AID_QREPLY,
                        UT3270_FALSE, out, out_limit, out_len);
                }
            else if (partition == 0x00 && qtype == UT3270_SNA_CMD_RMA)
                {
                return ut3270_build_read_modified(runtime, UT3270_AID_QREPLY,
                        UT3270_TRUE, out, out_limit, out_len);
                }
            else if (partition == 0x00 && qtype == UT3270_SNA_CMD_RB)
                {
                return ut3270_build_read_buffer(runtime, UT3270_AID_QREPLY,
                        out, out_limit, out_len);
                }
            }
        else if (sfid == UT3270_SF_ERASE_RESET && flen >= 4)
            {
            ut3270_runtime_reset_screen(runtime,
                    buf[i + 3] == UT3270_SF_ER_ALT ? UT3270_TRUE :
                    UT3270_TRUE);
            }
        else if (sfid == UT3270_SF_SET_REPLY_MODE && flen >= 4)
            {
            runtime->reply_mode = buf[i + 3];
            }
        else if (sfid == UT3270_SF_OUTBOUND_DS && flen >= 4)
            {
            ut3270_parse_3270(runtime, buf + i + 3, flen - 3, NULL, 0,
                    NULL);
            }
        else if (sfid == UT3270_SF_TRANSFER_DATA)
            {
            ut3270_set_status(runtime, "IND$FILE structured field received");
            }

        i += flen;
        }

    return 0;
    }

static int ut3270_query_reply(UT3270_RUNTIME *runtime,
        const unsigned char *wanted, int wanted_len, int all, unsigned char *out,
        int out_limit, int *out_len)
    {
    unsigned char payload[1024];
    int pos;
    int start;
    int i;
    int want_summary;
    int want_usable;
    int want_alpha;
    int want_charsets;
    int want_color;
    int want_highlight;
    int want_reply_modes;
    int want_imp;
    int include;
    static const unsigned char supported[] = {
        UT3270_QR_SUMMARY,
        UT3270_QR_USABLE_AREA,
        UT3270_QR_ALPHA_PART,
        UT3270_QR_CHARSETS,
        UT3270_QR_COLOR,
        UT3270_QR_HIGHLIGHTING,
        UT3270_QR_REPLY_MODES,
        UT3270_QR_IMP_PART,
        UT3270_QR_NULL
    };

    pos = 0;
    payload[pos++] = UT3270_AID_SF;

    want_summary = all;
    want_usable = all;
    want_alpha = all;
    want_charsets = all;
    want_color = all;
    want_highlight = all;
    want_reply_modes = all;
    want_imp = all;

    for (i = 0; i < wanted_len; ++i)
        {
        switch (wanted[i])
            {
        case UT3270_QR_SUMMARY: want_summary = UT3270_TRUE; break;
        case UT3270_QR_USABLE_AREA: want_usable = UT3270_TRUE; break;
        case UT3270_QR_ALPHA_PART: want_alpha = UT3270_TRUE; break;
        case UT3270_QR_CHARSETS: want_charsets = UT3270_TRUE; break;
        case UT3270_QR_COLOR: want_color = UT3270_TRUE; break;
        case UT3270_QR_HIGHLIGHTING: want_highlight = UT3270_TRUE; break;
        case UT3270_QR_REPLY_MODES: want_reply_modes = UT3270_TRUE; break;
        case UT3270_QR_IMP_PART: want_imp = UT3270_TRUE; break;
        default: break;
            }
        }

    if (!all && wanted_len == 0)
        {
        start = pos;
        pos += 2;
        payload[pos++] = UT3270_SFID_QREPLY;
        payload[pos++] = UT3270_QR_NULL;
        ut3270_set16(payload, start, pos - start);
        return ut3270_wrap_3270_payload(runtime, payload, pos, out,
                out_limit, out_len);
        }

#define UT3270_QR_BEGIN(code) \
    start = pos; \
    pos += 2; \
    payload[pos++] = UT3270_SFID_QREPLY; \
    payload[pos++] = (code)
#define UT3270_QR_END() ut3270_set16(payload, start, pos - start)
#define UT3270_QR_ADD(b) payload[pos++] = (unsigned char)(b)
#define UT3270_QR_ADD16(v) do { \
    payload[pos++] = (unsigned char)(((v) >> 8) & 0xff); \
    payload[pos++] = (unsigned char)((v) & 0xff); \
    } while (0)
#define UT3270_QR_ADD32(v) do { \
    payload[pos++] = (unsigned char)(((v) >> 24) & 0xff); \
    payload[pos++] = (unsigned char)(((v) >> 16) & 0xff); \
    payload[pos++] = (unsigned char)(((v) >> 8) & 0xff); \
    payload[pos++] = (unsigned char)((v) & 0xff); \
    } while (0)

    if (want_summary)
        {
        UT3270_QR_BEGIN(UT3270_QR_SUMMARY);
        for (i = 0; i < (int)sizeof(supported); ++i)
            UT3270_QR_ADD(supported[i]);
        UT3270_QR_END();
        }

    if (want_usable)
        {
        UT3270_QR_BEGIN(UT3270_QR_USABLE_AREA);
        UT3270_QR_ADD(0x01);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD16(runtime->snap.cols);
        UT3270_QR_ADD16(runtime->snap.rows);
        UT3270_QR_ADD(0x01);
        UT3270_QR_ADD32(0x000a02e5UL);
        UT3270_QR_ADD32(0x000e01b0UL);
        UT3270_QR_ADD(0x09);
        UT3270_QR_ADD(0x16);
        UT3270_QR_ADD16(runtime->cells);
        UT3270_QR_END();
        }

    if (want_alpha)
        {
        UT3270_QR_BEGIN(UT3270_QR_ALPHA_PART);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD16(runtime->cells);
        UT3270_QR_ADD(0x00);
        UT3270_QR_END();
        }

    if (want_charsets)
        {
        UT3270_QR_BEGIN(UT3270_QR_CHARSETS);
        UT3270_QR_ADD(0x82);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x09);
        UT3270_QR_ADD(0x16);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x07);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x10);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD32(0x03b10136UL);
        UT3270_QR_ADD(0x01);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0xf1);
        UT3270_QR_ADD32(0x03c30136UL);
        UT3270_QR_END();
        }

    if (want_color)
        {
        UT3270_QR_BEGIN(UT3270_QR_COLOR);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x10);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(UT3270_COLOR_GREEN);
        for (i = UT3270_COLOR_BLUE; i <= UT3270_COLOR_WHITE; ++i)
            {
            include = i;
            UT3270_QR_ADD(include);
            UT3270_QR_ADD(include);
            }
        UT3270_QR_END();
        }

    if (want_highlight)
        {
        UT3270_QR_BEGIN(UT3270_QR_HIGHLIGHTING);
        UT3270_QR_ADD(5);
        UT3270_QR_ADD(UT3270_XAH_DEFAULT);
        UT3270_QR_ADD(UT3270_XAH_NORMAL);
        UT3270_QR_ADD(UT3270_XAH_BLINK);
        UT3270_QR_ADD(UT3270_XAH_BLINK);
        UT3270_QR_ADD(UT3270_XAH_REVERSE);
        UT3270_QR_ADD(UT3270_XAH_REVERSE);
        UT3270_QR_ADD(UT3270_XAH_UNDERSCORE);
        UT3270_QR_ADD(UT3270_XAH_UNDERSCORE);
        UT3270_QR_ADD(UT3270_XAH_INTENSIFY);
        UT3270_QR_ADD(UT3270_XAH_INTENSIFY);
        UT3270_QR_END();
        }

    if (want_reply_modes)
        {
        UT3270_QR_BEGIN(UT3270_QR_REPLY_MODES);
        UT3270_QR_ADD(UT3270_SF_SRM_FIELD);
        UT3270_QR_ADD(UT3270_SF_SRM_XFIELD);
        UT3270_QR_ADD(UT3270_SF_SRM_CHAR);
        UT3270_QR_END();
        }

    if (want_imp)
        {
        UT3270_QR_BEGIN(UT3270_QR_IMP_PART);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD(0x0b);
        UT3270_QR_ADD(0x01);
        UT3270_QR_ADD(0x00);
        UT3270_QR_ADD16(80);
        UT3270_QR_ADD16(24);
        UT3270_QR_ADD16(runtime->snap.cols);
        UT3270_QR_ADD16(runtime->snap.rows);
        UT3270_QR_END();
        }

#undef UT3270_QR_BEGIN
#undef UT3270_QR_END
#undef UT3270_QR_ADD
#undef UT3270_QR_ADD16
#undef UT3270_QR_ADD32

    ut3270_set_status(runtime, "Structured-field query reply generated");
    return ut3270_wrap_3270_payload(runtime, payload, pos, out, out_limit,
            out_len);
    }

static int ut3270_build_read_modified(UT3270_RUNTIME *runtime,
        unsigned char aid, int all, unsigned char *out, int out_limit,
        int *out_len)
    {
    unsigned char payload[UT3270_TX_LIMIT];
    unsigned char addrbuf[2];
    int pos;
    int i;
    int run;

    pos = 0;
    payload[pos++] = aid;
    ut3270_encode_baddr(addrbuf, runtime->snap.cursor_addr,
            runtime->snap.rows, runtime->snap.cols);
    payload[pos++] = addrbuf[0];
    payload[pos++] = addrbuf[1];

    for (i = 0; i < runtime->cells && pos < (UT3270_TX_LIMIT - 8); ++i)
        {
        if (runtime->snap.cells[i].field_start)
            continue;
        if (!all && !runtime->snap.cells[i].modified)
            continue;

        payload[pos++] = UT3270_ORDER_SBA;
        ut3270_encode_baddr(addrbuf, i, runtime->snap.rows,
                runtime->snap.cols);
        payload[pos++] = addrbuf[0];
        payload[pos++] = addrbuf[1];
        run = i;
        while (run < runtime->cells &&
                !runtime->snap.cells[run].field_start &&
                (all || runtime->snap.cells[run].modified) &&
                pos < (UT3270_TX_LIMIT - 4))
            {
            payload[pos++] = runtime->snap.cells[run].ebc;
            runtime->snap.cells[run].modified = 0;
            ++run;
            }
        i = run - 1;
        }

    ut3270_set_status(runtime, "Read modified response generated");
    return ut3270_wrap_3270_payload(runtime, payload, pos, out, out_limit,
            out_len);
    }

static int ut3270_build_read_buffer(UT3270_RUNTIME *runtime,
        unsigned char aid, unsigned char *out, int out_limit, int *out_len)
    {
    unsigned char payload[UT3270_TX_LIMIT];
    unsigned char addrbuf[2];
    int pos;
    int i;

    pos = 0;
    payload[pos++] = aid;
    ut3270_encode_baddr(addrbuf, runtime->snap.cursor_addr,
            runtime->snap.rows, runtime->snap.cols);
    payload[pos++] = addrbuf[0];
    payload[pos++] = addrbuf[1];

    for (i = 0; i < runtime->cells && pos < (UT3270_TX_LIMIT - 4); ++i)
        {
        if (runtime->snap.cells[i].field_start)
            payload[pos++] = runtime->snap.cells[i].field_attr;
        else
            payload[pos++] = runtime->snap.cells[i].ebc;
        }

    ut3270_set_status(runtime, "Read buffer response generated");
    return ut3270_wrap_3270_payload(runtime, payload, pos, out, out_limit,
            out_len);
    }

static void ut3270_parse_bind(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len)
    {
    int ss;
    int rd;
    int cd;
    int ra;
    int ca;
    int namelen;
    int i;

    if (buf == NULL || len < 1 || buf[0] != UT3270_BIND_RU)
        {
        ut3270_set_status(runtime, "IBM 3270 BIND image ignored");
        return;
        }

    rd = runtime->snap.rows;
    cd = runtime->snap.cols;
    ra = runtime->snap.rows;
    ca = runtime->snap.cols;

    if (len > UT3270_BIND_OFF_SSIZE)
        {
        ss = buf[UT3270_BIND_OFF_SSIZE];
        switch (ss)
            {
        case 0x00:
        case 0x02:
            rd = 24; cd = 80; ra = 24; ca = 80;
            break;
        case 0x03:
            rd = 24; cd = 80;
            ra = runtime->snap.rows; ca = runtime->snap.cols;
            break;
        case 0x7e:
            if (len > UT3270_BIND_OFF_CD)
                {
                rd = buf[UT3270_BIND_OFF_RD];
                cd = buf[UT3270_BIND_OFF_CD];
                ra = rd;
                ca = cd;
                }
            break;
        case 0x7f:
            if (len > UT3270_BIND_OFF_CA)
                {
                rd = buf[UT3270_BIND_OFF_RD];
                cd = buf[UT3270_BIND_OFF_CD];
                ra = buf[UT3270_BIND_OFF_RA];
                ca = buf[UT3270_BIND_OFF_CA];
                }
            break;
        default:
            break;
            }
        }

    if (ra >= 24 && ca >= 80 && ra <= UT3270_MAX_ROWS &&
            ca <= UT3270_MAX_COLS)
        {
        ut3270_runtime_resize(runtime, ra, ca);
        ut3270_runtime_reset_screen(runtime, UT3270_TRUE);
        }
    else if (rd >= 24 && cd >= 80 && rd <= UT3270_MAX_ROWS &&
            cd <= UT3270_MAX_COLS)
        {
        ut3270_runtime_resize(runtime, rd, cd);
        ut3270_runtime_reset_screen(runtime, UT3270_TRUE);
        }

    runtime->plu_name[0] = '\0';
    if (len > UT3270_BIND_OFF_PLU_NAME_LEN)
        {
        namelen = buf[UT3270_BIND_OFF_PLU_NAME_LEN];
        if (namelen > UT3270_BIND_PLU_NAME_MAX)
            namelen = UT3270_BIND_PLU_NAME_MAX;
        if (namelen > 0 && len > UT3270_BIND_OFF_PLU_NAME + namelen)
            {
            for (i = 0; i < namelen; ++i)
                runtime->plu_name[i] = (char)ut3270_ebc_to_ascii(
                        buf[UT3270_BIND_OFF_PLU_NAME + i]);
            runtime->plu_name[namelen] = '\0';
            }
        }

    ut3270_set_status(runtime, "IBM 3270 BIND applied");
    }

static void ut3270_parse_scs(UT3270_RUNTIME *runtime,
        const unsigned char *buf, int len)
    {
    int i;
    unsigned char c;

    if (buf == NULL)
        return;

    for (i = 0; i < len; ++i)
        {
        c = buf[i];
        switch (c)
            {
        case UT3270_SCS_NL:
        case UT3270_SCS_LF:
            runtime->snap.buffer_addr += runtime->snap.cols -
                    (runtime->snap.buffer_addr % runtime->snap.cols);
            runtime->snap.buffer_addr %= runtime->cells;
            break;
        case UT3270_SCS_CR:
            runtime->snap.buffer_addr -=
                    runtime->snap.buffer_addr % runtime->snap.cols;
            break;
        case UT3270_SCS_BS:
            runtime->snap.buffer_addr = runtime->snap.buffer_addr == 0 ?
                    runtime->cells - 1 : runtime->snap.buffer_addr - 1;
            break;
        case UT3270_SCS_BEL:
            ut3270_set_status(runtime, "SCS bell");
            break;
        default:
            if (c >= 0x40)
                {
                ut3270_put_cell(runtime, runtime->snap.buffer_addr, c);
                ut3270_increment_addr(runtime);
                }
            break;
            }
        }
    }

static void ut3270_put_cell(UT3270_RUNTIME *runtime, int addr,
        unsigned char ebc)
    {
    if (addr < 0 || addr >= runtime->cells)
        return;

    runtime->snap.cells[addr].ebc = ebc;
    runtime->snap.cells[addr].ascii = (unsigned char)ut3270_ebc_to_ascii(ebc);
    runtime->snap.cells[addr].field_start = 0;
    runtime->snap.cells[addr].field_attr = 0;
    runtime->snap.cells[addr].highlight =
            (unsigned char)runtime->current_highlight;
    runtime->snap.cells[addr].foreground = (unsigned char)runtime->current_fg;
    runtime->snap.cells[addr].background = (unsigned char)runtime->current_bg;
    runtime->snap.cells[addr].charset = (unsigned char)runtime->current_charset;
    runtime->snap.cells[addr].input_control = (unsigned char)runtime->current_ic;
    }

static void ut3270_increment_addr(UT3270_RUNTIME *runtime)
    {
    runtime->snap.buffer_addr++;
    if (runtime->snap.buffer_addr >= runtime->cells)
        runtime->snap.buffer_addr = 0;
    }

static int ut3270_field_start_at(const UT3270_RUNTIME *runtime, int addr)
    {
    int i;

    if (runtime->cells <= 0)
        return -1;

    addr %= runtime->cells;
    for (i = addr; i >= 0; --i)
        {
        if (runtime->snap.cells[i].field_start)
            return i;
        }
    for (i = runtime->cells - 1; i > addr; --i)
        {
        if (runtime->snap.cells[i].field_start)
            return i;
        }
    return -1;
    }

static int ut3270_field_attr_at(const UT3270_RUNTIME *runtime, int addr)
    {
    int field_addr;

    field_addr = ut3270_field_start_at(runtime, addr);
    if (field_addr < 0)
        return 0;
    return runtime->snap.cells[field_addr].field_attr & UT3270_FA_MASK;
    }

static int ut3270_is_protected(const UT3270_RUNTIME *runtime, int addr)
    {
    return UT3270_FA_IS_PROTECTED(ut3270_field_attr_at(runtime, addr)) ?
            UT3270_TRUE : UT3270_FALSE;
    }

static int ut3270_is_numeric(const UT3270_RUNTIME *runtime, int addr)
    {
    return UT3270_FA_IS_NUMERIC(ut3270_field_attr_at(runtime, addr)) ?
            UT3270_TRUE : UT3270_FALSE;
    }

static void ut3270_set_mdt(UT3270_RUNTIME *runtime, int addr)
    {
    int field_addr;

    field_addr = ut3270_field_start_at(runtime, addr);
    if (field_addr >= 0)
        runtime->snap.cells[field_addr].field_attr |= UT3270_FA_MODIFY;
    }

static int ut3270_next_unprotected(const UT3270_RUNTIME *runtime, int addr,
        int direction)
    {
    int i;
    int next;

    next = addr;
    for (i = 0; i < runtime->cells; ++i)
        {
        next += direction;
        if (next < 0)
            next = runtime->cells - 1;
        else if (next >= runtime->cells)
            next = 0;

        if (!runtime->snap.cells[next].field_start &&
                !ut3270_is_protected(runtime, next))
            return next;
        }

    return addr;
    }

static void ut3270_erase_unprotected(UT3270_RUNTIME *runtime)
    {
    int i;

    for (i = 0; i < runtime->cells; ++i)
        {
        if (!runtime->snap.cells[i].field_start &&
                !ut3270_is_protected(runtime, i))
            {
            runtime->snap.cells[i].ebc = 0x40;
            runtime->snap.cells[i].ascii = ' ';
            runtime->snap.cells[i].modified = 0;
            }
        }
    runtime->snap.cursor_addr = ut3270_next_unprotected(runtime, 0, 1);
    }

static void ut3270_erase_unprotected_to(UT3270_RUNTIME *runtime, int stop_addr)
    {
    int addr;

    addr = runtime->snap.buffer_addr;
    stop_addr %= runtime->cells;
    while (addr != stop_addr)
        {
        if (!runtime->snap.cells[addr].field_start &&
                !ut3270_is_protected(runtime, addr))
            {
            runtime->snap.cells[addr].ebc = 0x40;
            runtime->snap.cells[addr].ascii = ' ';
            runtime->snap.cells[addr].modified = 0;
            }
        addr++;
        if (addr >= runtime->cells)
            addr = 0;
        }
    runtime->snap.buffer_addr = stop_addr;
    }

static void ut3270_erase_field(UT3270_RUNTIME *runtime, int addr)
    {
    int field_start;
    int i;

    if (ut3270_is_protected(runtime, addr))
        return;

    field_start = ut3270_field_start_at(runtime, addr);
    if (field_start < 0)
        field_start = 0;

    i = field_start + 1;
    if (i >= runtime->cells)
        i = 0;
    while (i != field_start)
        {
        if (runtime->snap.cells[i].field_start)
            break;
        runtime->snap.cells[i].ebc = 0x40;
        runtime->snap.cells[i].ascii = ' ';
        runtime->snap.cells[i].modified = 1;
        i++;
        if (i >= runtime->cells)
            i = 0;
        }
    ut3270_set_mdt(runtime, addr);
    runtime->snap.cursor_addr = field_start + 1;
    if (runtime->snap.cursor_addr >= runtime->cells)
        runtime->snap.cursor_addr = 0;
    }

static void ut3270_erase_eof(UT3270_RUNTIME *runtime, int addr)
    {
    int i;

    if (ut3270_is_protected(runtime, addr))
        return;

    i = addr;
    while (i < runtime->cells && !runtime->snap.cells[i].field_start)
        {
        runtime->snap.cells[i].ebc = 0x40;
        runtime->snap.cells[i].ascii = ' ';
        runtime->snap.cells[i].modified = 1;
        ++i;
        }
    ut3270_set_mdt(runtime, addr);
    }

static void ut3270_modify_current_attribute(UT3270_RUNTIME *runtime,
        unsigned char attr_type, unsigned char attr_value)
    {
    switch (attr_type)
        {
    case UT3270_XA_ALL:
        runtime->current_highlight = UT3270_XAH_DEFAULT;
        runtime->current_fg = UT3270_XAC_DEFAULT;
        runtime->current_bg = UT3270_XAC_DEFAULT;
        runtime->current_charset = 0;
        runtime->current_ic = UT3270_XAI_ENABLED;
        break;
    case UT3270_XA_3270:
        runtime->current_fa = attr_value & UT3270_FA_MASK;
        break;
    case UT3270_XA_HIGHLIGHTING:
        runtime->current_highlight = attr_value;
        break;
    case UT3270_XA_FOREGROUND:
        runtime->current_fg = attr_value;
        break;
    case UT3270_XA_BACKGROUND:
        runtime->current_bg = attr_value;
        break;
    case UT3270_XA_CHARSET:
        runtime->current_charset = attr_value;
        break;
    case UT3270_XA_INPUT_CONTROL:
        runtime->current_ic = attr_value;
        break;
    default:
        break;
        }
    }

static void ut3270_modify_field_attribute(UT3270_RUNTIME *runtime, int addr,
        unsigned char attr_type, unsigned char attr_value)
    {
    int field_addr;

    field_addr = ut3270_field_start_at(runtime, addr);
    if (field_addr < 0)
        return;

    switch (attr_type)
        {
    case UT3270_XA_3270:
        runtime->snap.cells[field_addr].field_attr =
                attr_value & UT3270_FA_MASK;
        break;
    case UT3270_XA_HIGHLIGHTING:
        runtime->snap.cells[field_addr].highlight = attr_value;
        break;
    case UT3270_XA_FOREGROUND:
        runtime->snap.cells[field_addr].foreground = attr_value;
        break;
    case UT3270_XA_BACKGROUND:
        runtime->snap.cells[field_addr].background = attr_value;
        break;
    case UT3270_XA_CHARSET:
        runtime->snap.cells[field_addr].charset = attr_value;
        break;
    case UT3270_XA_INPUT_CONTROL:
        runtime->snap.cells[field_addr].input_control = attr_value;
        break;
    case UT3270_XA_ALL:
        runtime->snap.cells[field_addr].highlight = UT3270_XAH_DEFAULT;
        runtime->snap.cells[field_addr].foreground = UT3270_XAC_DEFAULT;
        runtime->snap.cells[field_addr].background = UT3270_XAC_DEFAULT;
        runtime->snap.cells[field_addr].charset = 0;
        runtime->snap.cells[field_addr].input_control = UT3270_XAI_ENABLED;
        break;
    default:
        break;
        }
    }

static int ut3270_send_aid(UT3270_RUNTIME *runtime, unsigned char aid,
        unsigned char *out, int out_limit, int *out_len)
    {
    unsigned char payload[UT3270_TX_LIMIT];
    unsigned char addrbuf[2];
    int pos;
    int i;

    if (out_len)
        *out_len = 0;

    pos = 0;
    payload[pos++] = aid;
    if (!ut3270_is_short_aid(aid))
        {
        ut3270_encode_baddr(addrbuf, runtime->snap.cursor_addr,
                runtime->snap.rows, runtime->snap.cols);
        payload[pos++] = addrbuf[0];
        payload[pos++] = addrbuf[1];

        for (i = 0; i < runtime->cells && pos < (UT3270_TX_LIMIT - 8); ++i)
            {
            if (!runtime->snap.cells[i].field_start &&
                    runtime->snap.cells[i].modified)
                {
                payload[pos++] = UT3270_ORDER_SBA;
                ut3270_encode_baddr(addrbuf, i, runtime->snap.rows,
                        runtime->snap.cols);
                payload[pos++] = addrbuf[0];
                payload[pos++] = addrbuf[1];
                while (i < runtime->cells &&
                        !runtime->snap.cells[i].field_start &&
                        runtime->snap.cells[i].modified &&
                        pos < (UT3270_TX_LIMIT - 4))
                    {
                    payload[pos++] = runtime->snap.cells[i].ebc;
                    runtime->snap.cells[i].modified = 0;
                    ++i;
                    }
                --i;
                }
            }
        }

    ut3270_set_status(runtime, "AID 0x%02x sent", aid);
    return ut3270_wrap_3270_payload(runtime, payload, pos, out, out_limit,
            out_len);
    }

static int ut3270_is_short_aid(unsigned char aid)
    {
    return (aid == UT3270_AID_PA1 ||
            aid == UT3270_AID_PA2 ||
            aid == UT3270_AID_PA3 ||
            aid == UT3270_AID_CLEAR ||
            aid == UT3270_AID_SYSREQ) ? UT3270_TRUE : UT3270_FALSE;
    }

static unsigned char ut3270_aid_from_action(int action)
    {
    int n;
    static const unsigned char pf[24] = {
        UT3270_AID_PF1, UT3270_AID_PF2, UT3270_AID_PF3, UT3270_AID_PF4,
        UT3270_AID_PF5, UT3270_AID_PF6, UT3270_AID_PF7, UT3270_AID_PF8,
        UT3270_AID_PF9, UT3270_AID_PF10, UT3270_AID_PF11, UT3270_AID_PF12,
        UT3270_AID_PF13, UT3270_AID_PF14, UT3270_AID_PF15, UT3270_AID_PF16,
        UT3270_AID_PF17, UT3270_AID_PF18, UT3270_AID_PF19, UT3270_AID_PF20,
        UT3270_AID_PF21, UT3270_AID_PF22, UT3270_AID_PF23, UT3270_AID_PF24
    };
    static const unsigned char pa[3] = {
        UT3270_AID_PA1, UT3270_AID_PA2, UT3270_AID_PA3
    };

    if (action == UT3270_KEY_ENTER)
        return UT3270_AID_ENTER;
    if (action == UT3270_KEY_CLEAR)
        return UT3270_AID_CLEAR;
    if (action == UT3270_KEY_SYSREQ)
        return UT3270_AID_SYSREQ;
    if (action >= UT3270_KEY_PF_BASE + 1 &&
            action <= UT3270_KEY_PF_BASE + 24)
        {
        n = action - UT3270_KEY_PF_BASE - 1;
        return pf[n];
        }
    if (action >= UT3270_KEY_PA_BASE + 1 &&
            action <= UT3270_KEY_PA_BASE + 3)
        {
        n = action - UT3270_KEY_PA_BASE - 1;
        return pa[n];
        }
    return UT3270_AID_NO;
    }

static int ut3270_insert_char(UT3270_RUNTIME *runtime, int ch)
    {
    int addr;

    addr = runtime->snap.cursor_addr;
    if (ut3270_is_protected(runtime, addr) ||
            runtime->snap.cells[addr].input_control == UT3270_XAI_DISABLED)
        {
        runtime->keyboard_locked = UT3270_TRUE;
        runtime->snap.status_flags |= UT3270_STATUS_KEYBOARD_LOCKED;
        ut3270_set_status(runtime, "Protected field");
        return -1;
        }

    if (ut3270_is_numeric(runtime, addr) &&
            !((ch >= '0' && ch <= '9') || ch == '.' || ch == '-' ||
              ch == ' '))
        {
        runtime->keyboard_locked = UT3270_TRUE;
        runtime->snap.status_flags |= UT3270_STATUS_KEYBOARD_LOCKED;
        ut3270_set_status(runtime, "Numeric field");
        return -1;
        }

    runtime->snap.cells[addr].ebc = ut3270_ascii_to_ebc(ch);
    runtime->snap.cells[addr].ascii = (unsigned char)ch;
    runtime->snap.cells[addr].modified = 1;
    ut3270_set_mdt(runtime, addr);
    runtime->snap.cursor_addr = ut3270_next_unprotected(runtime, addr, 1);
    return 0;
    }

int ut3270_ebc_to_ascii(unsigned char ebc)
    {
    if (ebc >= 0xf0 && ebc <= 0xf9)
        return '0' + (ebc - 0xf0);
    if (ebc >= 0xc1 && ebc <= 0xc9)
        return 'A' + (ebc - 0xc1);
    if (ebc >= 0xd1 && ebc <= 0xd9)
        return 'J' + (ebc - 0xd1);
    if (ebc >= 0xe2 && ebc <= 0xe9)
        return 'S' + (ebc - 0xe2);
    if (ebc >= 0x81 && ebc <= 0x89)
        return 'a' + (ebc - 0x81);
    if (ebc >= 0x91 && ebc <= 0x99)
        return 'j' + (ebc - 0x91);
    if (ebc >= 0xa2 && ebc <= 0xa9)
        return 's' + (ebc - 0xa2);

    switch (ebc)
        {
    case 0x40: return ' ';
    case 0x4b: return '.';
    case 0x6b: return ',';
    case 0x5a: return '!';
    case 0x7a: return ':';
    case 0x7b: return '#';
    case 0x7c: return '@';
    case 0x7d: return '\'';
    case 0x7e: return '=';
    case 0x6c: return '%';
    case 0x50: return '&';
    case 0x60: return '-';
    case 0x61: return '/';
    case 0x4e: return '+';
    case 0x4c: return '<';
    case 0x6e: return '>';
    case 0x4d: return '(';
    case 0x5d: return ')';
    case 0x5b: return '$';
    case 0x7f: return '"';
    case 0x6d: return '_';
    case 0x5e: return ';';
    case 0x6f: return '?';
    case 0x4f: return '|';
    case 0x5f: return '^';
    case 0x4a: return '[';
    case UT3270_FC_DUP: return '*';
    case UT3270_FC_FM: return ';';
    default:
        return ' ';
        }
    }

unsigned char ut3270_ascii_to_ebc(int ch)
    {
    if (ch >= '0' && ch <= '9')
        return (unsigned char)(0xf0 + (ch - '0'));
    if (ch >= 'A' && ch <= 'I')
        return (unsigned char)(0xc1 + (ch - 'A'));
    if (ch >= 'J' && ch <= 'R')
        return (unsigned char)(0xd1 + (ch - 'J'));
    if (ch >= 'S' && ch <= 'Z')
        return (unsigned char)(0xe2 + (ch - 'S'));
    if (ch >= 'a' && ch <= 'i')
        return (unsigned char)(0x81 + (ch - 'a'));
    if (ch >= 'j' && ch <= 'r')
        return (unsigned char)(0x91 + (ch - 'j'));
    if (ch >= 's' && ch <= 'z')
        return (unsigned char)(0xa2 + (ch - 's'));

    switch (ch)
        {
    case ' ': return 0x40;
    case '.': return 0x4b;
    case ',': return 0x6b;
    case '!': return 0x5a;
    case ':': return 0x7a;
    case '#': return 0x7b;
    case '@': return 0x7c;
    case '\'': return 0x7d;
    case '=': return 0x7e;
    case '%': return 0x6c;
    case '&': return 0x50;
    case '-': return 0x60;
    case '/': return 0x61;
    case '+': return 0x4e;
    case '<': return 0x4c;
    case '>': return 0x6e;
    case '(': return 0x4d;
    case ')': return 0x5d;
    case '$': return 0x5b;
    case '"': return 0x7f;
    case '_': return 0x6d;
    case ';': return 0x5e;
    case '?': return 0x6f;
    case '|': return 0x4f;
    case '^': return 0x5f;
    case '[': return 0x4a;
    default:
        return 0x40;
        }
    }

static int ut3270_host_color_to_vc(unsigned char color)
    {
    switch (color)
        {
    case UT3270_COLOR_BLUE: return 1;
    case UT3270_COLOR_RED: return 4;
    case UT3270_COLOR_PINK: return 13;
    case UT3270_COLOR_GREEN: return 2;
    case UT3270_COLOR_TURQUOISE: return 3;
    case UT3270_COLOR_YELLOW: return 14;
    case UT3270_COLOR_BLACK:
    case UT3270_COLOR_NEUTRAL_BLACK: return 0;
    case UT3270_COLOR_DEEP_BLUE: return 9;
    case UT3270_COLOR_ORANGE: return 12;
    case UT3270_COLOR_PURPLE: return 5;
    case UT3270_COLOR_PALE_GREEN: return 10;
    case UT3270_COLOR_PALE_TURQUOISE: return 11;
    case UT3270_COLOR_GREY: return 8;
    case UT3270_COLOR_WHITE:
    case UT3270_COLOR_NEUTRAL_WHITE: return 15;
    default: return -1;
        }
    }

static int ut3270_is_format_control(unsigned char c)
    {
    switch (c)
        {
    case UT3270_FC_NULL:
    case UT3270_FC_FF:
    case UT3270_FC_CR:
    case UT3270_FC_SO:
    case UT3270_FC_SI:
    case UT3270_FC_NL:
    case UT3270_FC_EM:
    case UT3270_FC_LF:
    case UT3270_FC_DUP:
    case UT3270_FC_FM:
    case UT3270_FC_SUB:
    case UT3270_FC_EO:
        return UT3270_TRUE;
    default:
        return UT3270_FALSE;
        }
    }

static void ut3270_safe_copy(char *dst, int dst_len, const char *src)
    {
    if (dst == NULL || dst_len <= 0)
        return;
    if (src == NULL)
        src = "";
    strncpy(dst, src, (size_t)(dst_len - 1));
    dst[dst_len - 1] = '\0';
    }

static int ut3270_build_printer_command(UT3270_RUNTIME *runtime,
        const char *helper, char *cmd, int cmd_len)
    {
    const char *lu;
    int n;

    if (helper == NULL || cmd == NULL || cmd_len <= 0)
        return -1;

    lu = runtime->settings.printer_lu[0] ?
            runtime->settings.printer_lu : runtime->settings.lu_name;
    n = snprintf(cmd, (size_t)cmd_len, "\"%s\" --brand UltraTerminal%s%s%s%s",
            helper,
            lu[0] ? " --lu \"" : "",
            lu[0] ? lu : "",
            lu[0] ? "\"" : "",
            runtime->settings.printer_options[0] ? " " : "");
    if (n < 0 || n >= cmd_len)
        return -1;
    if (runtime->settings.printer_options[0])
        {
        strncat(cmd, runtime->settings.printer_options,
                (size_t)(cmd_len - (int)strlen(cmd) - 1));
        }
    return 0;
    }

int ut3270_runtime_host_color_to_vc_for_tests(unsigned char color)
    {
    return ut3270_host_color_to_vc(color);
    }
