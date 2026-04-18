/*
 * UltraTerminal Videotex runtime glue.
 *
 * Derived from vidtex-master's GPLv3 decoder/profile behavior, with the
 * ncurses/socket frontend removed so UltraTerminal can own rendering and I/O.
 * See third_party/vidtex/COPYING and UPSTREAM-README.md.
 */

#ifndef ULTRA_VIDTEX_H
#define ULTRA_VIDTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#define UTVIDTEX_ROWS 24
#define UTVIDTEX_COLS 40
#define UTVIDTEX_CELLS (UTVIDTEX_ROWS * UTVIDTEX_COLS)
#define UTVIDTEX_FRAME_BUFFER_MAX 2000
#define UTVIDTEX_MAX_PROFILE_NAME 32
#define UTVIDTEX_MAX_HOST 128
#define UTVIDTEX_MAX_AMBLE 16
#define UTVIDTEX_TX_LIMIT 64

#define UTVIDTEX_FONT_BEDSTEAD 0
#define UTVIDTEX_FONT_GALAX 1

#define UTVIDTEX_CELL_MOSAIC        0x0001
#define UTVIDTEX_CELL_FLASH         0x0002
#define UTVIDTEX_CELL_CONCEAL       0x0004
#define UTVIDTEX_CELL_DOUBLE_TOP    0x0008
#define UTVIDTEX_CELL_DOUBLE_BOTTOM 0x0010
#define UTVIDTEX_CELL_HELD_MOSAIC   0x0020

enum utvideotex_color
    {
    UTVIDTEX_BLACK = 0,
    UTVIDTEX_RED = 1,
    UTVIDTEX_GREEN = 2,
    UTVIDTEX_YELLOW = 3,
    UTVIDTEX_BLUE = 4,
    UTVIDTEX_MAGENTA = 5,
    UTVIDTEX_CYAN = 6,
    UTVIDTEX_WHITE = 7,
    UTVIDTEX_NONE = -1
    };

typedef struct utvideotex_cell
    {
    unsigned short ch;
    unsigned char foreground;
    unsigned char background;
    unsigned short flags;
    } UTVIDTEX_CELL;

typedef struct utvideotex_snapshot
    {
    UTVIDTEX_CELL cells[UTVIDTEX_CELLS];
    unsigned char raw_frame[UTVIDTEX_FRAME_BUFFER_MAX];
    int raw_frame_len;
    int rows;
    int cols;
    int cursor_row;
    int cursor_col;
    int cursor_on;
    int reveal_on;
    int flash_on;
    } UTVIDTEX_SNAPSHOT;

typedef struct utvideotex_settings
    {
    char profile_name[UTVIDTEX_MAX_PROFILE_NAME];
    char host[UTVIDTEX_MAX_HOST];
    int port;
    int font_map;
    int telnet_compat;
    unsigned char preamble[UTVIDTEX_MAX_AMBLE];
    int preamble_len;
    unsigned char postamble[UTVIDTEX_MAX_AMBLE];
    int postamble_len;
    } UTVIDTEX_SETTINGS;

typedef struct utvideotex_profile
    {
    const char *name;
    const char *host;
    int port;
    const unsigned char *preamble;
    int preamble_len;
    const unsigned char *postamble;
    int postamble_len;
    int telnet_compat;
    } UTVIDTEX_PROFILE;

typedef struct utvideotex_runtime UTVIDTEX_RUNTIME;

int utvideotex_profile_count(void);
const UTVIDTEX_PROFILE *utvideotex_profile_at(int index);
const UTVIDTEX_PROFILE *utvideotex_profile_by_name(const char *name);
void utvideotex_settings_defaults(UTVIDTEX_SETTINGS *settings);
void utvideotex_settings_apply_profile(UTVIDTEX_SETTINGS *settings,
        const UTVIDTEX_PROFILE *profile);

int utvideotex_runtime_create(UTVIDTEX_RUNTIME **runtime,
        const UTVIDTEX_SETTINGS *settings);
void utvideotex_runtime_destroy(UTVIDTEX_RUNTIME *runtime);
int utvideotex_runtime_connected(UTVIDTEX_RUNTIME *runtime,
        unsigned char *outbuf, int outcap, int *outlen);
int utvideotex_runtime_disconnecting(UTVIDTEX_RUNTIME *runtime,
        unsigned char *outbuf, int outcap, int *outlen);
int utvideotex_runtime_feed(UTVIDTEX_RUNTIME *runtime,
        const unsigned char *buf, int len);
int utvideotex_runtime_snapshot(UTVIDTEX_RUNTIME *runtime,
        UTVIDTEX_SNAPSHOT *snapshot);
int utvideotex_runtime_toggle_reveal(UTVIDTEX_RUNTIME *runtime);
int utvideotex_runtime_toggle_flash(UTVIDTEX_RUNTIME *runtime);
int utvideotex_runtime_key(UTVIDTEX_RUNTIME *runtime, int key,
        unsigned char *outbuf, int outcap, int *outlen);

#ifdef __cplusplus
}
#endif

#endif
