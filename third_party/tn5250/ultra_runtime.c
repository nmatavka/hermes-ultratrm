/*
 * UltraTerminal embedded IBM 5250/TN5250 runtime.
 */
#include "ultra_runtime.h"
#include "ultra_stream.h"

#include "lib5250/tn5250-private.h"

#include <stdarg.h>

#if defined(_WIN32)
#include <windows.h>
#endif

typedef struct ut5250_terminal_private
    {
    int rows;
    int cols;
    } UT5250_TERMINAL_PRIVATE;

struct ut5250_runtime
    {
    UT5250_SETTINGS settings;
    Tn5250Config* config;
    Tn5250Display* display;
    Tn5250Session* session;
    Tn5250Stream* stream;
    int connected;
    int printer_running;
    int tls_active;
    char status_text[128];
    };

static Tn5250Terminal* ut5250_terminal_new(int rows, int cols);
static void ut5250_terminal_init(Tn5250Terminal* terminal);
static void ut5250_terminal_term(Tn5250Terminal* terminal);
static void ut5250_terminal_destroy(Tn5250Terminal* terminal);
static int ut5250_terminal_width(Tn5250Terminal* terminal);
static int ut5250_terminal_height(Tn5250Terminal* terminal);
static int ut5250_terminal_flags(Tn5250Terminal* terminal);
static void ut5250_terminal_update(Tn5250Terminal* terminal,
        Tn5250Display* display);
static void ut5250_terminal_update_indicators(Tn5250Terminal* terminal,
        Tn5250Display* display);
static int ut5250_terminal_waitevent(Tn5250Terminal* terminal);
static int ut5250_terminal_getkey(Tn5250Terminal* terminal);
static void ut5250_terminal_putkey(Tn5250Terminal* terminal,
        Tn5250Display* display, unsigned char key, int row, int column);
static void ut5250_terminal_beep(Tn5250Terminal* terminal);
static int ut5250_terminal_enhanced(Tn5250Terminal* terminal);
static int ut5250_terminal_config(Tn5250Terminal* terminal,
        Tn5250Config* config);
static void ut5250_terminal_window_noop(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Window* window);
static void ut5250_terminal_scrollbar_create(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Scrollbar* scrollbar);
static void ut5250_terminal_scrollbar_destroy(Tn5250Terminal* terminal,
        Tn5250Display* display);
static void ut5250_terminal_menubar_noop(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Menubar* menubar);
static void ut5250_terminal_menuitem_noop(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Menuitem* menuitem);

static void ut5250_set_status(UT5250_RUNTIME* runtime, const char* fmt, ...);
static void ut5250_safe_copy(char* dst, int dst_len, const char* src);
static void ut5250_configure(UT5250_RUNTIME* runtime);
static int ut5250_map_key(int action);
static unsigned char ut5250_cell_flags(unsigned char attr, Tn5250Field* field);
static unsigned char ut5250_cell_color(unsigned char attr);
static void ut5250_build_printer_command(UT5250_RUNTIME* runtime,
        const char* helper, char* cmd, int cmd_len);

static const unsigned char ut5250_attr_flags[32] = {
    0,
    UT5250_CELL_REVERSE,
    0,
    UT5250_CELL_REVERSE,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_REVERSE | UT5250_CELL_UNDERLINE,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_BLANK,
    0,
    UT5250_CELL_REVERSE,
    UT5250_CELL_BLINK,
    UT5250_CELL_REVERSE | UT5250_CELL_BLINK,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_REVERSE | UT5250_CELL_UNDERLINE,
    UT5250_CELL_UNDERLINE | UT5250_CELL_BLINK,
    UT5250_CELL_BLANK,
    0,
    UT5250_CELL_REVERSE,
    0,
    UT5250_CELL_REVERSE,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_REVERSE | UT5250_CELL_UNDERLINE,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_REVERSE | UT5250_CELL_BLANK,
    0,
    UT5250_CELL_REVERSE,
    0,
    UT5250_CELL_REVERSE,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_REVERSE | UT5250_CELL_UNDERLINE,
    UT5250_CELL_UNDERLINE,
    UT5250_CELL_BLANK
};

static const unsigned char ut5250_attr_color[32] = {
    2, 2, 15, 15, 2, 2, 15, 0,
    12, 12, 12, 12, 12, 12, 12, 0,
    11, 11, 14, 14, 11, 11, 14, 0,
    13, 13, 11, 11, 13, 13, 11, 0
};

void ut5250_settings_defaults(UT5250_SETTINGS* settings)
    {
    if (settings == NULL)
        return;

    memset(settings, 0, sizeof(*settings));
    settings->tls_mode = UT5250_TLS_OFF;
    settings->verify_tls = 1;
    ut5250_safe_copy(settings->terminal_type,
            sizeof(settings->terminal_type), "IBM-3179-2");
    ut5250_safe_copy(settings->host_codepage,
            sizeof(settings->host_codepage), "37");
    }

int ut5250_runtime_create(UT5250_RUNTIME** out_runtime,
        const UT5250_SETTINGS* settings)
    {
    UT5250_RUNTIME* runtime;
    UT5250_SETTINGS defaults;
    Tn5250Terminal* terminal;
    char tls_error[128];

    if (out_runtime == NULL)
        return -1;

    *out_runtime = NULL;
    runtime = (UT5250_RUNTIME*)malloc(sizeof(UT5250_RUNTIME));
    if (runtime == NULL)
        return -1;

    memset(runtime, 0, sizeof(*runtime));
    if (settings)
        runtime->settings = *settings;
    else
        {
        ut5250_settings_defaults(&defaults);
        runtime->settings = defaults;
        }

    runtime->config = tn5250_config_new();
    runtime->display = tn5250_display_new();
    runtime->session = tn5250_session_new();
    if (runtime->config == NULL || runtime->display == NULL ||
            runtime->session == NULL)
        {
        ut5250_runtime_destroy(runtime);
        return -1;
        }

    ut5250_configure(runtime);

    if (tn5250_display_config(runtime->display, runtime->config) == -1 ||
            tn5250_session_config(runtime->session, runtime->config) == -1)
        {
        ut5250_runtime_destroy(runtime);
        return -1;
        }

    terminal = ut5250_terminal_new(24, 80);
    if (terminal == NULL)
        {
        ut5250_runtime_destroy(runtime);
        return -1;
        }
    tn5250_terminal_config(terminal, runtime->config);
    tn5250_terminal_init(terminal);
    tn5250_display_set_terminal(runtime->display, terminal);
    tn5250_display_set_session(runtime->display, runtime->session);

    runtime->stream = tn5250_stream_open("ultra:ultraterminal",
            runtime->config);
    if (runtime->stream == NULL)
        {
        ut5250_runtime_destroy(runtime);
        return -1;
        }
    if (runtime->settings.tls_mode == UT5250_TLS_DIRECT &&
            tn5250_ultra_stream_enable_tls(runtime->stream,
                runtime->settings.verify_tls,
                runtime->settings.ca_path,
                runtime->settings.cert_path,
                runtime->settings.key_path,
                tls_error,
                sizeof(tls_error)) < 0)
        {
        ut5250_set_status(runtime, "%s", tls_error);
        ut5250_runtime_destroy(runtime);
        return -1;
        }
    tn5250_session_set_stream(runtime->session, runtime->stream);
    ut5250_set_status(runtime, "IBM 5250 ready");

    *out_runtime = runtime;
    return 0;
    }

void ut5250_runtime_destroy(UT5250_RUNTIME* runtime)
    {
    if (runtime == NULL)
        return;

    if (runtime->session)
        {
        tn5250_session_destroy(runtime->session);
        runtime->session = NULL;
        runtime->stream = NULL;
        }
    if (runtime->display)
        {
        tn5250_display_destroy(runtime->display);
        runtime->display = NULL;
        }
    if (runtime->config)
        {
        tn5250_config_unref(runtime->config);
        runtime->config = NULL;
        }
    free(runtime);
    }

int ut5250_runtime_connected(UT5250_RUNTIME* runtime, unsigned char* out,
        int out_limit, int* out_len)
    {
    int rc;

    if (out_len)
        *out_len = 0;
    if (runtime == NULL || runtime->stream == NULL)
        return -1;

    runtime->connected = 1;
    rc = tn5250_ultra_stream_start(runtime->stream, out, out_limit, out_len);
    if (runtime->settings.tls_mode == UT5250_TLS_DIRECT)
        ut5250_set_status(runtime,
                "IBM 5250 TLS handshake started");
    else
        ut5250_set_status(runtime, "IBM 5250 connected");
    return rc;
    }

int ut5250_runtime_feed_bytes(UT5250_RUNTIME* runtime,
        const unsigned char* data, int data_len, unsigned char* out,
        int out_limit, int* out_len)
    {
    int rc;

    if (out_len)
        *out_len = 0;
    if (runtime == NULL || runtime->stream == NULL || data == NULL ||
            data_len <= 0)
        return -1;

    rc = tn5250_ultra_stream_feed(runtime->stream, data, data_len, out,
            out_limit, out_len);
    if (rc < 0)
        {
        tn5250_ultra_stream_end_output(runtime->stream);
        return rc;
        }

    tn5250_session_pump_pending(runtime->session);
    runtime->tls_active = tn5250_ultra_stream_tls_active(runtime->stream);
    if (runtime->tls_active)
        ut5250_set_status(runtime, "IBM 5250 TLS active");
    tn5250_ultra_stream_end_output(runtime->stream);
    return 0;
    }

int ut5250_runtime_key(UT5250_RUNTIME* runtime, int action, int ch,
        unsigned char* out, int out_limit, int* out_len)
    {
    int key;

    if (out_len)
        *out_len = 0;
    if (runtime == NULL || runtime->display == NULL ||
            runtime->stream == NULL)
        return -1;

    if (tn5250_ultra_stream_begin_output(runtime->stream, out, out_limit,
            out_len) < 0)
        return -1;

    if (action == UT5250_KEY_CHAR)
        key = ch;
    else
        key = ut5250_map_key(action);

    if (key != K_UNKNOW)
        tn5250_display_do_key(runtime->display, key);

    tn5250_ultra_stream_end_output(runtime->stream);
    return (key == K_UNKNOW) ? -1 : 0;
    }

int ut5250_runtime_snapshot(UT5250_RUNTIME* runtime,
        UT5250_SNAPSHOT* snapshot)
    {
    Tn5250DBuffer* dbuf;
    Tn5250CharMap* map;
    Tn5250Field* field;
    int row;
    int col;
    int addr;
    int rows;
    int cols;
    unsigned char raw;
    unsigned char current_attr;
    UT5250_CELL* cell;

    if (runtime == NULL || runtime->display == NULL || snapshot == NULL)
        return -1;

    memset(snapshot, 0, sizeof(*snapshot));
    dbuf = tn5250_display_dbuffer(runtime->display);
    map = tn5250_display_char_map(runtime->display);
    rows = tn5250_display_height(runtime->display);
    cols = tn5250_display_width(runtime->display);
    if (rows <= 0 || rows > UT5250_MAX_ROWS)
        rows = 24;
    if (cols <= 0 || cols > UT5250_MAX_COLS)
        cols = 80;

    snapshot->rows = rows;
    snapshot->cols = cols;
    snapshot->cursor_row = tn5250_dbuffer_cursor_y(dbuf);
    snapshot->cursor_col = tn5250_dbuffer_cursor_x(dbuf);
    snapshot->status_flags = 0;
    if (runtime->connected)
        snapshot->status_flags |= UT5250_STATUS_CONNECTED;
    if (runtime->settings.tls_mode == UT5250_TLS_DIRECT)
        snapshot->status_flags |= UT5250_STATUS_TLS_REQUESTED;
    if (runtime->tls_active)
        snapshot->status_flags |= UT5250_STATUS_TLS_ACTIVE;
    if (tn5250_display_inhibited(runtime->display))
        snapshot->status_flags |= UT5250_STATUS_KEYBOARD_LOCKED;
    if (runtime->printer_running)
        snapshot->status_flags |= UT5250_STATUS_PRINTER_RUNNING;
    ut5250_safe_copy(snapshot->status_text, sizeof(snapshot->status_text),
            runtime->status_text);

    current_attr = 0x20;
    for (row = 0; row < rows; ++row)
        {
        for (col = 0; col < cols; ++col)
            {
            addr = row * cols + col;
            cell = &snapshot->cells[addr];
            raw = tn5250_display_char_at(runtime->display, row, col);
            field = tn5250_display_field_at(runtime->display, row, col);

            cell->raw = raw;
            cell->field_attr = current_attr;
            cell->foreground = ut5250_cell_color(current_attr);
            cell->background = 0;
            cell->flags = ut5250_cell_flags(current_attr, field);

            if ((raw & 0xe0) == 0x20)
                {
                current_attr = raw;
                cell->field_start = 1;
                cell->field_attr = raw;
                cell->ascii = ' ';
                cell->flags = ut5250_cell_flags(raw, field) |
                        UT5250_CELL_BLANK;
                cell->foreground = ut5250_cell_color(raw);
                }
            else if ((raw == 0x1f) || (raw == 0x3f) ||
                    (raw < 0x40 && raw > 0x00) || raw == 0xff ||
                    (cell->flags & UT5250_CELL_BLANK))
                {
                cell->ascii = ' ';
                }
            else if (map)
                {
                cell->ascii = tn5250_char_map_to_local(map, raw);
                }
            else
                {
                cell->ascii = raw;
                }
            }
        }

    return 0;
    }

int ut5250_runtime_start_printer(UT5250_RUNTIME* runtime, const char* helper,
        char* message, int message_len)
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
        ut5250_safe_copy(message, message_len,
                "UltraTerminal 5250 printer is already running.");
        return 0;
        }

    ut5250_build_printer_command(runtime, helper, cmd, sizeof(cmd));
#if defined(_WIN32)
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
        ut5250_safe_copy(message, message_len,
                "Could not launch UltraTerminal5250Printer.exe.");
        return -1;
        }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
#else
    (void)cmd;
#endif

    runtime->printer_running = 1;
    ut5250_set_status(runtime, "5250 printer helper started");
    ut5250_safe_copy(message, message_len,
            "UltraTerminal 5250 printer helper started.");
    return 0;
    }

int ut5250_runtime_stop_printer(UT5250_RUNTIME* runtime, char* message,
        int message_len)
    {
    if (runtime == NULL)
        return -1;
    runtime->printer_running = 0;
    ut5250_set_status(runtime, "5250 printer helper stopped");
    ut5250_safe_copy(message, message_len,
            "UltraTerminal marked the 5250 printer helper stopped.");
    return 0;
    }

const char* ut5250_runtime_status(UT5250_RUNTIME* runtime)
    {
    if (runtime == NULL)
        return "";
    return runtime->status_text;
    }

static void ut5250_configure(UT5250_RUNTIME* runtime)
    {
    const char* term;
    const char* map;

    term = runtime->settings.terminal_type[0] ?
            runtime->settings.terminal_type : "IBM-3179-2";
    map = runtime->settings.host_codepage[0] ?
            runtime->settings.host_codepage : "37";

    tn5250_config_set(runtime->config, "env.TERM", term);
    tn5250_config_set(runtime->config, "map", map);
    tn5250_config_set(runtime->config, "destructive_backspace", "1");
    tn5250_config_set(runtime->config, "uninhibited", "1");

    if (runtime->settings.device_name[0])
        tn5250_config_set(runtime->config, "env.DEVNAME",
                runtime->settings.device_name);
    if (runtime->settings.verify_tls)
        tn5250_config_set(runtime->config, "ssl_verify_server", "1");
    if (runtime->settings.ca_path[0])
        tn5250_config_set(runtime->config, "ssl_ca_file",
                runtime->settings.ca_path);
    if (runtime->settings.cert_path[0])
        tn5250_config_set(runtime->config, "ssl_cert_file",
                runtime->settings.cert_path);
    }

static int ut5250_map_key(int action)
    {
    if (action >= UT5250_KEY_PF_BASE + 1 &&
            action <= UT5250_KEY_PF_BASE + 24)
        return K_F1 + (action - UT5250_KEY_PF_BASE - 1);

    switch (action)
        {
    case UT5250_KEY_ENTER:
        return K_ENTER;
    case UT5250_KEY_CLEAR:
        return K_CLEAR;
    case UT5250_KEY_RESET:
        return K_RESET;
    case UT5250_KEY_HELP:
        return K_HELP;
    case UT5250_KEY_ATTENTION:
        return K_ATTENTION;
    case UT5250_KEY_SYSREQ:
        return K_SYSREQ;
    case UT5250_KEY_ROLLUP:
        return K_ROLLUP;
    case UT5250_KEY_ROLLDN:
        return K_ROLLDN;
    case UT5250_KEY_FIELD_EXIT:
        return K_FIELDEXIT;
    case UT5250_KEY_FIELD_PLUS:
        return K_FIELDPLUS;
    case UT5250_KEY_FIELD_MINUS:
        return K_FIELDMINUS;
    case UT5250_KEY_DUP:
        return K_DUPLICATE;
    case UT5250_KEY_INSERT:
        return K_INSERT;
    case UT5250_KEY_DELETE:
        return K_DELETE;
    case UT5250_KEY_BACKSPACE:
        return K_BACKSPACE;
    case UT5250_KEY_TAB:
        return K_TAB;
    case UT5250_KEY_BACKTAB:
        return K_BACKTAB;
    case UT5250_KEY_LEFT:
        return K_LEFT;
    case UT5250_KEY_RIGHT:
        return K_RIGHT;
    case UT5250_KEY_UP:
        return K_UP;
    case UT5250_KEY_DOWN:
        return K_DOWN;
    case UT5250_KEY_HOME:
        return K_HOME;
    case UT5250_KEY_END:
        return K_END;
    case UT5250_KEY_PRINT:
        return K_PRINT;
    default:
        return K_UNKNOW;
        }
    }

static unsigned char ut5250_cell_flags(unsigned char attr, Tn5250Field* field)
    {
    unsigned char flags;
    int index;

    if (attr < 0x20 || attr > 0x3f)
        attr = 0x20;
    index = attr - 0x20;
    flags = ut5250_attr_flags[index];
    if (field)
        {
        if (tn5250_field_is_bypass(field))
            flags |= UT5250_CELL_PROTECTED;
        if (tn5250_field_mdt(field))
            flags |= UT5250_CELL_MODIFIED;
        }
    return flags;
    }

static unsigned char ut5250_cell_color(unsigned char attr)
    {
    if (attr < 0x20 || attr > 0x3f)
        attr = 0x20;
    return ut5250_attr_color[attr - 0x20];
    }

static void ut5250_set_status(UT5250_RUNTIME* runtime, const char* fmt, ...)
    {
    va_list ap;

    if (runtime == NULL)
        return;
    va_start(ap, fmt);
    vsnprintf(runtime->status_text, sizeof(runtime->status_text), fmt, ap);
    runtime->status_text[sizeof(runtime->status_text) - 1] = '\0';
    va_end(ap);
    }

static void ut5250_safe_copy(char* dst, int dst_len, const char* src)
    {
    if (dst == NULL || dst_len <= 0)
        return;
    if (src == NULL)
        src = "";
    strncpy(dst, src, dst_len - 1);
    dst[dst_len - 1] = '\0';
    }

static void ut5250_build_printer_command(UT5250_RUNTIME* runtime,
        const char* helper, char* cmd, int cmd_len)
    {
    const char* exe;
    const char* device;

    exe = (helper && helper[0]) ? helper : "UltraTerminal5250Printer.exe";
    device = runtime->settings.printer_device[0] ?
            runtime->settings.printer_device : runtime->settings.device_name;
    snprintf(cmd, cmd_len, "\"%s\" --device \"%s\" --map \"%s\" %s",
            exe,
            device ? device : "",
            runtime->settings.host_codepage[0] ?
                runtime->settings.host_codepage : "37",
            runtime->settings.printer_options);
    cmd[cmd_len - 1] = '\0';
    }

static Tn5250Terminal* ut5250_terminal_new(int rows, int cols)
    {
    Tn5250Terminal* terminal;
    UT5250_TERMINAL_PRIVATE* data;

    terminal = (Tn5250Terminal*)malloc(sizeof(Tn5250Terminal));
    data = (UT5250_TERMINAL_PRIVATE*)malloc(sizeof(UT5250_TERMINAL_PRIVATE));
    if (terminal == NULL || data == NULL)
        {
        if (terminal)
            free(terminal);
        if (data)
            free(data);
        return NULL;
        }

    memset(terminal, 0, sizeof(*terminal));
    memset(data, 0, sizeof(*data));
    data->rows = rows;
    data->cols = cols;
    terminal->data = (struct _Tn5250TerminalPrivate*)data;
    terminal->init = ut5250_terminal_init;
    terminal->term = ut5250_terminal_term;
    terminal->destroy = ut5250_terminal_destroy;
    terminal->width = ut5250_terminal_width;
    terminal->height = ut5250_terminal_height;
    terminal->flags = ut5250_terminal_flags;
    terminal->update = ut5250_terminal_update;
    terminal->update_indicators = ut5250_terminal_update_indicators;
    terminal->waitevent = ut5250_terminal_waitevent;
    terminal->getkey = ut5250_terminal_getkey;
    terminal->putkey = ut5250_terminal_putkey;
    terminal->beep = ut5250_terminal_beep;
    terminal->enhanced = ut5250_terminal_enhanced;
    terminal->config = ut5250_terminal_config;
    terminal->create_window = ut5250_terminal_window_noop;
    terminal->destroy_window = ut5250_terminal_window_noop;
    terminal->create_scrollbar = ut5250_terminal_scrollbar_create;
    terminal->destroy_scrollbar = ut5250_terminal_scrollbar_destroy;
    terminal->create_menubar = ut5250_terminal_menubar_noop;
    terminal->destroy_menubar = ut5250_terminal_menubar_noop;
    terminal->create_menuitem = ut5250_terminal_menuitem_noop;
    terminal->destroy_menuitem = ut5250_terminal_menuitem_noop;
    return terminal;
    }

static void ut5250_terminal_init(Tn5250Terminal* terminal)
    {
    (void)terminal;
    }

static void ut5250_terminal_term(Tn5250Terminal* terminal)
    {
    (void)terminal;
    }

static void ut5250_terminal_destroy(Tn5250Terminal* terminal)
    {
    if (terminal)
        {
        if (terminal->data)
            free(terminal->data);
        free(terminal);
        }
    }

static int ut5250_terminal_width(Tn5250Terminal* terminal)
    {
    UT5250_TERMINAL_PRIVATE* data;

    data = (UT5250_TERMINAL_PRIVATE*)terminal->data;
    return data ? data->cols : 80;
    }

static int ut5250_terminal_height(Tn5250Terminal* terminal)
    {
    UT5250_TERMINAL_PRIVATE* data;

    data = (UT5250_TERMINAL_PRIVATE*)terminal->data;
    return data ? data->rows : 24;
    }

static int ut5250_terminal_flags(Tn5250Terminal* terminal)
    {
    (void)terminal;
    return TN5250_TERMINAL_HAS_COLOR;
    }

static void ut5250_terminal_update(Tn5250Terminal* terminal,
        Tn5250Display* display)
    {
    (void)terminal;
    (void)display;
    }

static void ut5250_terminal_update_indicators(Tn5250Terminal* terminal,
        Tn5250Display* display)
    {
    (void)terminal;
    (void)display;
    }

static int ut5250_terminal_waitevent(Tn5250Terminal* terminal)
    {
    (void)terminal;
    return 0;
    }

static int ut5250_terminal_getkey(Tn5250Terminal* terminal)
    {
    (void)terminal;
    return K_UNKNOW;
    }

static void ut5250_terminal_putkey(Tn5250Terminal* terminal,
        Tn5250Display* display, unsigned char key, int row, int column)
    {
    (void)terminal;
    (void)display;
    (void)key;
    (void)row;
    (void)column;
    }

static void ut5250_terminal_beep(Tn5250Terminal* terminal)
    {
    (void)terminal;
    MessageBeep((UINT)-1);
    }

static int ut5250_terminal_enhanced(Tn5250Terminal* terminal)
    {
    (void)terminal;
    return 1;
    }

static int ut5250_terminal_config(Tn5250Terminal* terminal,
        Tn5250Config* config)
    {
    UT5250_TERMINAL_PRIVATE* data;
    const char* term;

    data = (UT5250_TERMINAL_PRIVATE*)terminal->data;
    if (data == NULL || config == NULL)
        return 0;

    term = tn5250_config_get(config, "env.TERM");
    if (term && strstr(term, "132"))
        {
        data->rows = 27;
        data->cols = 132;
        }
    else
        {
        data->rows = 24;
        data->cols = 80;
        }
    return 0;
    }

static void ut5250_terminal_window_noop(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Window* window)
    {
    (void)terminal;
    (void)display;
    (void)window;
    }

static void ut5250_terminal_scrollbar_create(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Scrollbar* scrollbar)
    {
    (void)terminal;
    (void)display;
    (void)scrollbar;
    }

static void ut5250_terminal_scrollbar_destroy(Tn5250Terminal* terminal,
        Tn5250Display* display)
    {
    (void)terminal;
    (void)display;
    }

static void ut5250_terminal_menubar_noop(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Menubar* menubar)
    {
    (void)terminal;
    (void)display;
    (void)menubar;
    }

static void ut5250_terminal_menuitem_noop(Tn5250Terminal* terminal,
        Tn5250Display* display, Tn5250Menuitem* menuitem)
    {
    (void)terminal;
    (void)display;
    (void)menuitem;
    }
