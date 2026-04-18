/*
 * UltraTerminal TN3270 runtime API.
 *
 * This is the UltraTerminal-facing glue boundary for the suite3270-derived
 * runtime. Protocol constants and screen-state behavior are adapted from
 * suite3270 4.5; see LICENSE.md for the BSD-style notice.
 */

#if !defined(ULTRATERMINAL_SUITE3270_RUNTIME_H)
#define ULTRATERMINAL_SUITE3270_RUNTIME_H

#include "tn3270_protocol.h"

#define UT3270_MODEL_DEFAULT 2
#define UT3270_MAX_ROWS 50
#define UT3270_MAX_COLS 132
#define UT3270_MAX_CELLS (UT3270_MAX_ROWS * UT3270_MAX_COLS)
#define UT3270_MAX_LU 64
#define UT3270_MAX_PATH_CHARS 260
#define UT3270_MAX_CODEPAGE 32
#define UT3270_MAX_PRINTER_OPTS 260
#define UT3270_TX_LIMIT 8192

#define UT3270_TLS_OFF 0
#define UT3270_TLS_DIRECT 1
#define UT3270_TLS_STARTTLS 2

#define UT3270_KEY_NONE 0
#define UT3270_KEY_CHAR 1
#define UT3270_KEY_ENTER 2
#define UT3270_KEY_CLEAR 3
#define UT3270_KEY_SYSREQ 4
#define UT3270_KEY_RESET 5
#define UT3270_KEY_TAB 6
#define UT3270_KEY_BACKTAB 7
#define UT3270_KEY_LEFT 8
#define UT3270_KEY_RIGHT 9
#define UT3270_KEY_UP 10
#define UT3270_KEY_DOWN 11
#define UT3270_KEY_DELETE 12
#define UT3270_KEY_BACKSPACE 13
#define UT3270_KEY_ERASE_FIELD 14
#define UT3270_KEY_ERASE_EOF 15
#define UT3270_KEY_DUP 16
#define UT3270_KEY_FIELD_MARK 17
#define UT3270_KEY_PF_BASE 100
#define UT3270_KEY_PA_BASE 200

#define UT3270_STATUS_CONNECTED 0x0001
#define UT3270_STATUS_TN3270E 0x0002
#define UT3270_STATUS_KEYBOARD_LOCKED 0x0004
#define UT3270_STATUS_PRINTER_RUNNING 0x0008
#define UT3270_STATUS_TLS_REQUESTED 0x0010
#define UT3270_STATUS_TLS_ACTIVE 0x0020

typedef struct ut3270_runtime UT3270_RUNTIME;

typedef struct ut3270_settings
    {
    int model;
    int tls_mode;
    int verify_tls;
    int printer_enabled;
    char lu_name[UT3270_MAX_LU];
    char device_name[UT3270_MAX_LU];
    char host_codepage[UT3270_MAX_CODEPAGE];
    char ca_path[UT3270_MAX_PATH_CHARS];
    char cert_path[UT3270_MAX_PATH_CHARS];
    char key_path[UT3270_MAX_PATH_CHARS];
    char indfile_dir[UT3270_MAX_PATH_CHARS];
    char indfile_options[UT3270_MAX_PRINTER_OPTS];
    char printer_lu[UT3270_MAX_LU];
    char printer_options[UT3270_MAX_PRINTER_OPTS];
    } UT3270_SETTINGS;

typedef struct ut3270_cell
    {
    unsigned char ebc;
    unsigned char ascii;
    unsigned char field_start;
    unsigned char field_attr;
    unsigned char highlight;
    unsigned char foreground;
    unsigned char background;
    unsigned char charset;
    unsigned char input_control;
    unsigned char modified;
    } UT3270_CELL;

typedef struct ut3270_snapshot
    {
    int rows;
    int cols;
    int cursor_addr;
    int buffer_addr;
    int status_flags;
    unsigned char reply_mode;
    char status_text[128];
    UT3270_CELL cells[UT3270_MAX_CELLS];
    } UT3270_SNAPSHOT;

void ut3270_settings_defaults(UT3270_SETTINGS *settings);
int ut3270_runtime_create(UT3270_RUNTIME **out_runtime,
        const UT3270_SETTINGS *settings);
void ut3270_runtime_destroy(UT3270_RUNTIME *runtime);
void ut3270_runtime_set_telnet_mode(UT3270_RUNTIME *runtime, int tn3270e);
int ut3270_runtime_feed_record(UT3270_RUNTIME *runtime,
        const unsigned char *record, int record_len,
        unsigned char *out_record, int out_limit, int *out_len);
int ut3270_runtime_key(UT3270_RUNTIME *runtime, int action, int ch,
        unsigned char *out_record, int out_limit, int *out_len);
int ut3270_runtime_snapshot(UT3270_RUNTIME *runtime,
        UT3270_SNAPSHOT *snapshot);
int ut3270_runtime_start_printer(UT3270_RUNTIME *runtime, const char *helper,
        char *message, int message_len);
int ut3270_runtime_stop_printer(UT3270_RUNTIME *runtime, char *message,
        int message_len);
int ut3270_runtime_indfile_send(UT3270_RUNTIME *runtime, const char *local,
        const char *remote, char *message, int message_len);
int ut3270_runtime_indfile_receive(UT3270_RUNTIME *runtime, const char *remote,
        const char *local, char *message, int message_len);
const char *ut3270_runtime_status(UT3270_RUNTIME *runtime);
int ut3270_ebc_to_ascii(unsigned char ebc);
unsigned char ut3270_ascii_to_ebc(int ch);

#endif
