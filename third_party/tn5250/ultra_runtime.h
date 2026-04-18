/*
 * UltraTerminal IBM 5250/TN5250 runtime API.
 */
#ifndef ULTRATERMINAL_TN5250_RUNTIME_H
#define ULTRATERMINAL_TN5250_RUNTIME_H

#define UT5250_MAX_ROWS 50
#define UT5250_MAX_COLS 132
#define UT5250_MAX_CELLS (UT5250_MAX_ROWS * UT5250_MAX_COLS)
#define UT5250_MAX_NAME 64
#define UT5250_MAX_PATH_CHARS 260
#define UT5250_MAX_OPTIONS 260
#define UT5250_TX_LIMIT 8192

#define UT5250_TLS_OFF 0
#define UT5250_TLS_DIRECT 1

#define UT5250_KEY_NONE 0
#define UT5250_KEY_CHAR 1
#define UT5250_KEY_ENTER 2
#define UT5250_KEY_CLEAR 3
#define UT5250_KEY_RESET 4
#define UT5250_KEY_HELP 5
#define UT5250_KEY_ATTENTION 6
#define UT5250_KEY_SYSREQ 7
#define UT5250_KEY_ROLLUP 8
#define UT5250_KEY_ROLLDN 9
#define UT5250_KEY_FIELD_EXIT 10
#define UT5250_KEY_FIELD_PLUS 11
#define UT5250_KEY_FIELD_MINUS 12
#define UT5250_KEY_DUP 13
#define UT5250_KEY_INSERT 14
#define UT5250_KEY_DELETE 15
#define UT5250_KEY_BACKSPACE 16
#define UT5250_KEY_TAB 17
#define UT5250_KEY_BACKTAB 18
#define UT5250_KEY_LEFT 19
#define UT5250_KEY_RIGHT 20
#define UT5250_KEY_UP 21
#define UT5250_KEY_DOWN 22
#define UT5250_KEY_HOME 23
#define UT5250_KEY_END 24
#define UT5250_KEY_PRINT 25
#define UT5250_KEY_PF_BASE 100

#define UT5250_STATUS_CONNECTED 0x0001
#define UT5250_STATUS_TLS_REQUESTED 0x0002
#define UT5250_STATUS_TLS_ACTIVE 0x0004
#define UT5250_STATUS_KEYBOARD_LOCKED 0x0008
#define UT5250_STATUS_PRINTER_RUNNING 0x0010

#define UT5250_CELL_UNDERLINE 0x01
#define UT5250_CELL_BLINK 0x02
#define UT5250_CELL_BLANK 0x04
#define UT5250_CELL_REVERSE 0x08
#define UT5250_CELL_PROTECTED 0x10
#define UT5250_CELL_MODIFIED 0x20

typedef struct ut5250_runtime UT5250_RUNTIME;

typedef struct ut5250_settings
    {
    int tls_mode;
    int verify_tls;
    int printer_enabled;
    char terminal_type[UT5250_MAX_NAME];
    char device_name[UT5250_MAX_NAME];
    char host_codepage[UT5250_MAX_NAME];
    char ca_path[UT5250_MAX_PATH_CHARS];
    char cert_path[UT5250_MAX_PATH_CHARS];
    char key_path[UT5250_MAX_PATH_CHARS];
    char printer_device[UT5250_MAX_NAME];
    char printer_options[UT5250_MAX_OPTIONS];
    } UT5250_SETTINGS;

typedef struct ut5250_cell
    {
    unsigned char raw;
    unsigned char ascii;
    unsigned char field_start;
    unsigned char field_attr;
    unsigned char flags;
    unsigned char foreground;
    unsigned char background;
    } UT5250_CELL;

typedef struct ut5250_snapshot
    {
    int rows;
    int cols;
    int cursor_row;
    int cursor_col;
    int status_flags;
    char status_text[128];
    UT5250_CELL cells[UT5250_MAX_CELLS];
    } UT5250_SNAPSHOT;

void ut5250_settings_defaults(UT5250_SETTINGS* settings);
int ut5250_runtime_create(UT5250_RUNTIME** out_runtime,
        const UT5250_SETTINGS* settings);
void ut5250_runtime_destroy(UT5250_RUNTIME* runtime);
int ut5250_runtime_connected(UT5250_RUNTIME* runtime, unsigned char* out,
        int out_limit, int* out_len);
int ut5250_runtime_feed_bytes(UT5250_RUNTIME* runtime,
        const unsigned char* data, int data_len, unsigned char* out,
        int out_limit, int* out_len);
int ut5250_runtime_key(UT5250_RUNTIME* runtime, int action, int ch,
        unsigned char* out, int out_limit, int* out_len);
int ut5250_runtime_snapshot(UT5250_RUNTIME* runtime,
        UT5250_SNAPSHOT* snapshot);
int ut5250_runtime_start_printer(UT5250_RUNTIME* runtime, const char* helper,
        char* message, int message_len);
int ut5250_runtime_stop_printer(UT5250_RUNTIME* runtime, char* message,
        int message_len);
const char* ut5250_runtime_status(UT5250_RUNTIME* runtime);

#endif
