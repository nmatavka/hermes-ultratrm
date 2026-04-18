/*
 * UltraTerminal Videotex runtime glue.
 *
 * Derived from vidtex-master's GPLv3 decoder/profile behavior. This keeps
 * the Mode 7/Viewdata data-stream semantics but removes ncurses, POSIX
 * sockets, poll loops, and file I/O so UltraTerminal owns those layers.
 */

#include "ultra_vidtex.h"

#include <stdlib.h>
#include <string.h>

#define TRI_FALSE 0
#define TRI_TRUE 1
#define TRI_UNDEF -1

typedef struct utvideotex_char
    {
    unsigned short single;
    unsigned short upper;
    unsigned short lower;
    } UTVIDTEX_CHAR;

typedef struct utvideotex_flags
    {
    int bg_color;
    int alpha_fg_color;
    int mosaic_fg_color;
    int is_alpha;
    int is_contiguous;
    int is_flashing;
    int is_escaped;
    int is_concealed;
    int is_mosaic_held;
    int is_double_height;
    int is_cursor_on;
    UTVIDTEX_CHAR held_mosaic;
    } UTVIDTEX_FLAGS;

typedef struct utvideotex_after_flags
    {
    int alpha_fg_color;
    int mosaic_fg_color;
    int is_flashing;
    int is_boxing;
    int is_mosaic_held;
    int is_double_height;
    } UTVIDTEX_AFTER_FLAGS;

struct utvideotex_runtime
    {
    UTVIDTEX_SETTINGS settings;
    UTVIDTEX_SNAPSHOT snapshot;
    UTVIDTEX_FLAGS flags;
    UTVIDTEX_AFTER_FLAGS after_flags;
    UTVIDTEX_CHAR space;
    int dheight_low_row;
    };

static const unsigned char telstar_telnet_preamble[] = { 255, 253, 3 };
static const unsigned char telstar_postamble[] = { 42, 57, 48, 95, 95 };
static const unsigned char ccl4_postamble[] = { 42, 81, 81 };
static const unsigned char teefax_postamble[] = { 70, 70, 70 };

static const UTVIDTEX_PROFILE profiles[] =
    {
    { "NXTel", "nx.nxtel.org", 23280, 0, 0, 0, 0, 0 },
    { "TeeFax", "teletext.matrixnetwork.co.uk", 6502, 0, 0,
        teefax_postamble, sizeof(teefax_postamble), 0 },
    { "Telstar fast", "glasstty.com", 6502,
        telstar_telnet_preamble, sizeof(telstar_telnet_preamble),
        telstar_postamble, sizeof(telstar_postamble), 1 },
    { "Telstar slow", "glasstty.com", 6502, 0, 0,
        telstar_postamble, sizeof(telstar_postamble), 0 },
    { "CCL4", "fish.ccl4.org", 23, 0, 0,
        ccl4_postamble, sizeof(ccl4_postamble), 0 },
    { "Night Owl", "nightowlbbs.ddns.net", 6400, 0, 0,
        telstar_postamble, sizeof(telstar_postamble), 0 }
    };

static int utvideotex_name_equals(const char *lhs, const char *rhs);
static void utvideotex_reset_flags(UTVIDTEX_RUNTIME *runtime);
static void utvideotex_reset_after_flags(UTVIDTEX_RUNTIME *runtime);
static void utvideotex_new_frame(UTVIDTEX_RUNTIME *runtime);
static void utvideotex_next_row(UTVIDTEX_RUNTIME *runtime);
static void utvideotex_fill_end(UTVIDTEX_RUNTIME *runtime);
static void utvideotex_apply_after_flags(UTVIDTEX_RUNTIME *runtime);
static void utvideotex_get_char_code(UTVIDTEX_RUNTIME *runtime,
        int is_alpha, int is_contiguous, int row_code, int col_code,
        UTVIDTEX_CHAR *ch);
static unsigned short utvideotex_bedstead_map(int row_code, int col_code,
        int is_alpha, int is_contiguous, int is_dheight,
        int is_dheight_lower);
static unsigned short utvideotex_galax_map(int row_code, int col_code,
        int is_alpha, int is_contiguous, int is_dheight,
        int is_dheight_lower);
static void utvideotex_put_char(UTVIDTEX_RUNTIME *runtime, int row, int col,
        const UTVIDTEX_CHAR *ch, int use_upper, int use_lower,
        unsigned short flags);
static void utvideotex_put_cell(UTVIDTEX_RUNTIME *runtime, int row, int col,
        unsigned short ch, unsigned short flags);
static int utvideotex_copy_bytes(unsigned char *outbuf, int outcap,
        int *outlen, const unsigned char *src, int srclen);
static int utvideotex_color_is_valid(int color);

int utvideotex_profile_count(void)
    {
    return (int)(sizeof(profiles) / sizeof(profiles[0]));
    }

const UTVIDTEX_PROFILE *utvideotex_profile_at(int index)
    {
    if (index < 0 || index >= utvideotex_profile_count())
        return 0;
    return &profiles[index];
    }

const UTVIDTEX_PROFILE *utvideotex_profile_by_name(const char *name)
    {
    int index;

    if (name == 0)
        return 0;

    for (index = 0; index < utvideotex_profile_count(); ++index)
        {
        if (utvideotex_name_equals(name, profiles[index].name))
            return &profiles[index];
        }
    return 0;
    }

void utvideotex_settings_defaults(UTVIDTEX_SETTINGS *settings)
    {
    if (settings == 0)
        return;

    memset(settings, 0, sizeof(*settings));
    settings->font_map = UTVIDTEX_FONT_BEDSTEAD;
    utvideotex_settings_apply_profile(settings, &profiles[0]);
    }

void utvideotex_settings_apply_profile(UTVIDTEX_SETTINGS *settings,
        const UTVIDTEX_PROFILE *profile)
    {
    int count;

    if (settings == 0 || profile == 0)
        return;

    strncpy(settings->profile_name, profile->name,
        sizeof(settings->profile_name) - 1);
    settings->profile_name[sizeof(settings->profile_name) - 1] = '\0';
    strncpy(settings->host, profile->host, sizeof(settings->host) - 1);
    settings->host[sizeof(settings->host) - 1] = '\0';
    settings->port = profile->port;
    settings->telnet_compat = profile->telnet_compat;

    memset(settings->preamble, 0, sizeof(settings->preamble));
    memset(settings->postamble, 0, sizeof(settings->postamble));
    settings->preamble_len = 0;
    settings->postamble_len = 0;

    count = profile->preamble_len;
    if (count > UTVIDTEX_MAX_AMBLE)
        count = UTVIDTEX_MAX_AMBLE;
    if (profile->preamble && count > 0)
        {
        memcpy(settings->preamble, profile->preamble, (size_t)count);
        settings->preamble_len = count;
        }

    count = profile->postamble_len;
    if (count > UTVIDTEX_MAX_AMBLE)
        count = UTVIDTEX_MAX_AMBLE;
    if (profile->postamble && count > 0)
        {
        memcpy(settings->postamble, profile->postamble, (size_t)count);
        settings->postamble_len = count;
        }
    }

int utvideotex_runtime_create(UTVIDTEX_RUNTIME **runtime,
        const UTVIDTEX_SETTINGS *settings)
    {
    UTVIDTEX_RUNTIME *created;

    if (runtime == 0)
        return -1;

    *runtime = 0;
    created = (UTVIDTEX_RUNTIME *)malloc(sizeof(*created));
    if (created == 0)
        return -1;

    memset(created, 0, sizeof(*created));
    if (settings)
        created->settings = *settings;
    else
        utvideotex_settings_defaults(&created->settings);

    if (created->settings.font_map != UTVIDTEX_FONT_GALAX)
        created->settings.font_map = UTVIDTEX_FONT_BEDSTEAD;

    created->snapshot.rows = UTVIDTEX_ROWS;
    created->snapshot.cols = UTVIDTEX_COLS;
    created->space.single = ' ';
    created->space.upper = ' ';
    created->space.lower = ' ';
    utvideotex_new_frame(created);

    *runtime = created;
    return 0;
    }

void utvideotex_runtime_destroy(UTVIDTEX_RUNTIME *runtime)
    {
    if (runtime)
        free(runtime);
    }

int utvideotex_runtime_connected(UTVIDTEX_RUNTIME *runtime,
        unsigned char *outbuf, int outcap, int *outlen)
    {
    unsigned char startup[UTVIDTEX_MAX_AMBLE + 1];
    int len;

    if (runtime == 0)
        return -1;

    if (outlen)
        *outlen = 0;

    startup[0] = 22;
    len = 1;
    if (runtime->settings.preamble_len > 0)
        {
        memcpy(&startup[1], runtime->settings.preamble,
            (size_t)runtime->settings.preamble_len);
        len += runtime->settings.preamble_len;
        }

    return utvideotex_copy_bytes(outbuf, outcap, outlen, startup, len);
    }

int utvideotex_runtime_disconnecting(UTVIDTEX_RUNTIME *runtime,
        unsigned char *outbuf, int outcap, int *outlen)
    {
    if (runtime == 0)
        return -1;

    return utvideotex_copy_bytes(outbuf, outcap, outlen,
        runtime->settings.postamble, runtime->settings.postamble_len);
    }

int utvideotex_runtime_feed(UTVIDTEX_RUNTIME *runtime,
        const unsigned char *buf, int len)
    {
    int index;

    if (runtime == 0 || buf == 0 || len <= 0)
        return -1;

    for (index = 0; index < len; ++index)
        {
        unsigned char b;
        int row_code;
        int col_code;
        int is_spacing_attribute;
        unsigned short cell_flags;
        UTVIDTEX_CHAR ch;

        b = buf[index];
        if (runtime->snapshot.raw_frame_len < UTVIDTEX_FRAME_BUFFER_MAX)
            {
            runtime->snapshot.raw_frame[runtime->snapshot.raw_frame_len] = b;
            ++runtime->snapshot.raw_frame_len;
            }

        switch (b)
            {
        case 0:
            continue;
        case 8:
            --runtime->snapshot.cursor_col;
            if (runtime->snapshot.cursor_col < 0)
                {
                runtime->snapshot.cursor_col = UTVIDTEX_COLS - 1;
                --runtime->snapshot.cursor_row;
                if (runtime->snapshot.cursor_row < 0)
                    runtime->snapshot.cursor_row = UTVIDTEX_ROWS - 1;
                }
            continue;
        case 9:
            ++runtime->snapshot.cursor_col;
            if (runtime->snapshot.cursor_col >= UTVIDTEX_COLS)
                {
                runtime->snapshot.cursor_col = 0;
                ++runtime->snapshot.cursor_row;
                if (runtime->snapshot.cursor_row >= UTVIDTEX_ROWS)
                    runtime->snapshot.cursor_row = 0;
                }
            continue;
        case 10:
            ++runtime->snapshot.cursor_row;
            if (runtime->snapshot.cursor_row >= UTVIDTEX_ROWS)
                runtime->snapshot.cursor_row = 0;
            utvideotex_reset_flags(runtime);
            utvideotex_reset_after_flags(runtime);
            continue;
        case 11:
            --runtime->snapshot.cursor_row;
            if (runtime->snapshot.cursor_row < 0)
                runtime->snapshot.cursor_row = UTVIDTEX_ROWS - 1;
            continue;
        case 12:
            utvideotex_new_frame(runtime);
            continue;
        case 13:
            utvideotex_fill_end(runtime);
            runtime->snapshot.cursor_col = 0;
            continue;
        case 17:
            runtime->flags.is_cursor_on = 1;
            runtime->snapshot.cursor_on = 1;
            continue;
        case 20:
            runtime->flags.is_cursor_on = 0;
            runtime->snapshot.cursor_on = 0;
            continue;
        case 30:
            utvideotex_fill_end(runtime);
            runtime->snapshot.cursor_col = 0;
            runtime->snapshot.cursor_row = 0;
            continue;
            }

        row_code = b & 0x0f;
        col_code = (b & 0x70) >> 4;

        if (runtime->flags.is_escaped)
            {
            col_code &= 1;
            runtime->flags.is_escaped = 0;
            }

        is_spacing_attribute = (col_code == 0 || col_code == 1);

        if (col_code == 0)
            {
            switch (row_code)
                {
            case 0:
            case 14:
            case 15:
                break;
            case 8:
                runtime->after_flags.is_flashing = TRI_TRUE;
                break;
            case 9:
                runtime->flags.is_flashing = 0;
                break;
            case 10:
                runtime->after_flags.is_boxing = TRI_FALSE;
                break;
            case 11:
                runtime->after_flags.is_boxing = TRI_TRUE;
                break;
            case 12:
                runtime->flags.is_double_height = 0;
                runtime->flags.held_mosaic = runtime->space;
                break;
            case 13:
                if (runtime->snapshot.cursor_row < (UTVIDTEX_ROWS - 2) &&
                        runtime->snapshot.cursor_row != runtime->dheight_low_row)
                    runtime->after_flags.is_double_height = TRI_TRUE;
                break;
            default:
                runtime->after_flags.alpha_fg_color = row_code;
                break;
                }
            }
        else if (col_code == 1)
            {
            switch (row_code)
                {
            case 0:
                break;
            case 8:
                runtime->flags.is_concealed = 1;
                break;
            case 9:
                runtime->flags.is_contiguous = 1;
                break;
            case 10:
                runtime->flags.is_contiguous = 0;
                break;
            case 11:
                runtime->flags.is_escaped = 1;
                continue;
            case 12:
                runtime->flags.bg_color = UTVIDTEX_BLACK;
                break;
            case 13:
                runtime->flags.bg_color = runtime->flags.is_alpha ?
                    runtime->flags.alpha_fg_color :
                    runtime->flags.mosaic_fg_color;
                break;
            case 14:
                runtime->flags.is_mosaic_held = 1;
                break;
            case 15:
                runtime->after_flags.is_mosaic_held = TRI_FALSE;
                break;
            default:
                runtime->after_flags.mosaic_fg_color = row_code;
                break;
                }
            }

        memset(&ch, 0, sizeof(ch));
        if (runtime->snapshot.cursor_row != runtime->dheight_low_row)
            {
            cell_flags = 0;
            if (runtime->flags.is_flashing)
                cell_flags |= UTVIDTEX_CELL_FLASH;
            if (runtime->flags.is_concealed)
                cell_flags |= UTVIDTEX_CELL_CONCEAL;
            if (!runtime->flags.is_alpha)
                cell_flags |= UTVIDTEX_CELL_MOSAIC;

            if (is_spacing_attribute)
                {
                if (runtime->flags.is_mosaic_held)
                    {
                    ch = runtime->flags.held_mosaic;
                    cell_flags |= UTVIDTEX_CELL_HELD_MOSAIC;
                    }
                else
                    {
                    ch = runtime->space;
                    }
                }
            else
                {
                utvideotex_get_char_code(runtime, runtime->flags.is_alpha,
                    runtime->flags.is_contiguous, row_code, col_code, &ch);
                if (!runtime->flags.is_alpha)
                    runtime->flags.held_mosaic = ch;
                }

            if (runtime->flags.is_double_height)
                {
                utvideotex_put_char(runtime, runtime->snapshot.cursor_row,
                    runtime->snapshot.cursor_col, &ch, 1, 0,
                    cell_flags | UTVIDTEX_CELL_DOUBLE_TOP);
                if (runtime->dheight_low_row >= 0 &&
                        runtime->dheight_low_row < UTVIDTEX_ROWS)
                    {
                    utvideotex_put_char(runtime, runtime->dheight_low_row,
                        runtime->snapshot.cursor_col, &ch, 0, 1,
                        cell_flags | UTVIDTEX_CELL_DOUBLE_BOTTOM);
                    }
                }
            else
                {
                utvideotex_put_char(runtime, runtime->snapshot.cursor_row,
                    runtime->snapshot.cursor_col, &ch, 0, 0, cell_flags);
                }
            }

        utvideotex_apply_after_flags(runtime);
        if (runtime->after_flags.is_double_height == TRI_TRUE)
            runtime->dheight_low_row = runtime->snapshot.cursor_row + 1;
        utvideotex_reset_after_flags(runtime);

        ++runtime->snapshot.cursor_col;
        if (runtime->snapshot.cursor_col == UTVIDTEX_COLS)
            utvideotex_next_row(runtime);
        }

    return 0;
    }

int utvideotex_runtime_snapshot(UTVIDTEX_RUNTIME *runtime,
        UTVIDTEX_SNAPSHOT *snapshot)
    {
    if (runtime == 0 || snapshot == 0)
        return -1;
    *snapshot = runtime->snapshot;
    return 0;
    }

int utvideotex_runtime_toggle_reveal(UTVIDTEX_RUNTIME *runtime)
    {
    if (runtime == 0)
        return -1;
    runtime->snapshot.reveal_on = !runtime->snapshot.reveal_on;
    return 0;
    }

int utvideotex_runtime_toggle_flash(UTVIDTEX_RUNTIME *runtime)
    {
    if (runtime == 0)
        return -1;
    runtime->snapshot.flash_on = !runtime->snapshot.flash_on;
    return 0;
    }

int utvideotex_runtime_key(UTVIDTEX_RUNTIME *runtime, int key,
        unsigned char *outbuf, int outcap, int *outlen)
    {
    unsigned char ch;

    if (runtime == 0)
        return -1;
    if (outlen)
        *outlen = 0;

    if (key == '#' || key == '\n' || key == '\r')
        ch = (unsigned char)'_';
    else if (key >= 0 && key <= 255)
        ch = (unsigned char)key;
    else
        return -1;

    return utvideotex_copy_bytes(outbuf, outcap, outlen, &ch, 1);
    }

static int utvideotex_name_equals(const char *lhs, const char *rhs)
    {
    unsigned char a;
    unsigned char b;

    if (lhs == 0 || rhs == 0)
        return 0;

    while (*lhs && *rhs)
        {
        a = (unsigned char)*lhs;
        b = (unsigned char)*rhs;
        if (a >= 'A' && a <= 'Z')
            a = (unsigned char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = (unsigned char)(b - 'A' + 'a');
        if (a != b)
            return 0;
        ++lhs;
        ++rhs;
        }
    return *lhs == '\0' && *rhs == '\0';
    }

static void utvideotex_reset_flags(UTVIDTEX_RUNTIME *runtime)
    {
    runtime->flags.bg_color = UTVIDTEX_BLACK;
    runtime->flags.alpha_fg_color = UTVIDTEX_WHITE;
    runtime->flags.mosaic_fg_color = UTVIDTEX_WHITE;
    runtime->flags.is_alpha = 1;
    runtime->flags.is_contiguous = 1;
    runtime->flags.is_flashing = 0;
    runtime->flags.is_escaped = 0;
    runtime->flags.is_concealed = 0;
    runtime->flags.is_mosaic_held = 0;
    runtime->flags.is_double_height = 0;
    runtime->flags.held_mosaic = runtime->space;
    }

static void utvideotex_reset_after_flags(UTVIDTEX_RUNTIME *runtime)
    {
    runtime->after_flags.alpha_fg_color = UTVIDTEX_NONE;
    runtime->after_flags.mosaic_fg_color = UTVIDTEX_NONE;
    runtime->after_flags.is_flashing = TRI_UNDEF;
    runtime->after_flags.is_boxing = TRI_UNDEF;
    runtime->after_flags.is_mosaic_held = TRI_UNDEF;
    runtime->after_flags.is_double_height = TRI_UNDEF;
    }

static void utvideotex_new_frame(UTVIDTEX_RUNTIME *runtime)
    {
    int row;
    int col;

    runtime->snapshot.cursor_row = 0;
    runtime->snapshot.cursor_col = 0;
    runtime->snapshot.raw_frame_len = 0;
    runtime->snapshot.reveal_on = 0;
    runtime->snapshot.flash_on = 1;
    runtime->dheight_low_row = -1;
    utvideotex_reset_flags(runtime);
    utvideotex_reset_after_flags(runtime);

    for (row = 0; row < UTVIDTEX_ROWS; ++row)
        {
        for (col = 0; col < UTVIDTEX_COLS; ++col)
            utvideotex_put_cell(runtime, row, col, ' ', 0);
        }
    }

static void utvideotex_next_row(UTVIDTEX_RUNTIME *runtime)
    {
    if ((runtime->snapshot.cursor_row + 1) < UTVIDTEX_ROWS)
        ++runtime->snapshot.cursor_row;
    else
        runtime->snapshot.cursor_row = 0;

    runtime->snapshot.cursor_col = 0;
    utvideotex_reset_flags(runtime);
    utvideotex_reset_after_flags(runtime);
    }

static void utvideotex_fill_end(UTVIDTEX_RUNTIME *runtime)
    {
    UTVIDTEX_CELL previous;
    int col;

    if (runtime->snapshot.cursor_col <= 0 ||
            runtime->snapshot.cursor_row < 0 ||
            runtime->snapshot.cursor_row >= UTVIDTEX_ROWS)
        return;

    previous = runtime->snapshot.cells[
        runtime->snapshot.cursor_row * UTVIDTEX_COLS +
        runtime->snapshot.cursor_col - 1];

    for (col = runtime->snapshot.cursor_col; col < UTVIDTEX_COLS; ++col)
        {
        UTVIDTEX_CELL *cell;

        cell = &runtime->snapshot.cells[
            runtime->snapshot.cursor_row * UTVIDTEX_COLS + col];
        cell->foreground = previous.foreground;
        cell->background = previous.background;
        cell->flags = previous.flags;
        }
    }

static void utvideotex_apply_after_flags(UTVIDTEX_RUNTIME *runtime)
    {
    int was_alpha;

    was_alpha = runtime->flags.is_alpha;

    if (runtime->after_flags.alpha_fg_color != UTVIDTEX_NONE)
        {
        runtime->flags.alpha_fg_color = runtime->after_flags.alpha_fg_color;
        runtime->flags.is_alpha = 1;
        runtime->flags.is_concealed = 0;
        }
    else if (runtime->after_flags.mosaic_fg_color != UTVIDTEX_NONE)
        {
        runtime->flags.mosaic_fg_color = runtime->after_flags.mosaic_fg_color;
        runtime->flags.is_alpha = 0;
        runtime->flags.is_concealed = 0;
        }

    if (runtime->flags.is_alpha != was_alpha)
        runtime->flags.held_mosaic = runtime->space;

    if (runtime->after_flags.is_flashing == TRI_TRUE)
        runtime->flags.is_flashing = 1;

    if (runtime->after_flags.is_mosaic_held == TRI_FALSE)
        runtime->flags.is_mosaic_held = 0;

    if (runtime->after_flags.is_double_height == TRI_TRUE)
        runtime->flags.is_double_height = 1;
    }

static void utvideotex_get_char_code(UTVIDTEX_RUNTIME *runtime,
        int is_alpha, int is_contiguous, int row_code, int col_code,
        UTVIDTEX_CHAR *ch)
    {
    if (runtime->settings.font_map == UTVIDTEX_FONT_GALAX)
        {
        ch->single = utvideotex_galax_map(row_code, col_code, is_alpha,
            is_contiguous, 0, 0);
        ch->upper = utvideotex_galax_map(row_code, col_code, is_alpha,
            is_contiguous, 1, 0);
        ch->lower = utvideotex_galax_map(row_code, col_code, is_alpha,
            is_contiguous, 1, 1);
        }
    else
        {
        ch->single = utvideotex_bedstead_map(row_code, col_code, is_alpha,
            is_contiguous, 0, 0);
        ch->upper = utvideotex_bedstead_map(row_code, col_code, is_alpha,
            is_contiguous, 1, 0);
        ch->lower = utvideotex_bedstead_map(row_code, col_code, is_alpha,
            is_contiguous, 1, 1);
        }
    }

static unsigned short utvideotex_bedstead_map(int row_code, int col_code,
        int is_alpha, int is_contiguous, int is_dheight,
        int is_dheight_lower)
    {
    int is_graph;

    is_graph = !is_alpha;
    if (row_code < 0 || row_code > 15 || col_code < 0 || col_code > 7)
        return 0x20;
    if (is_dheight && is_dheight_lower)
        return 0x20;

    if (col_code == 2 && is_alpha)
        return row_code == 3 ? 0x00A3 : (unsigned short)(0x20 + row_code);
    if (col_code == 2 && is_graph)
        return (unsigned short)((is_contiguous ? 0xEE00 : 0xEE20) + row_code);
    if (col_code == 3 && is_alpha)
        return (unsigned short)(0x30 + row_code);
    if (col_code == 3 && is_graph)
        return (unsigned short)((is_contiguous ? 0xEE10 : 0xEE30) + row_code);
    if (col_code == 4)
        return (unsigned short)(0x40 + row_code);
    if (col_code == 5)
        {
        if (row_code == 11) return 0x2190;
        if (row_code == 12) return 0x00BD;
        if (row_code == 13) return 0x2192;
        if (row_code == 14) return 0x2191;
        if (row_code == 15) return 0x0023;
        return (unsigned short)(0x50 + row_code);
        }
    if (col_code == 6 && is_alpha)
        return row_code == 0 ? 0x2013 : (unsigned short)(0x60 + row_code);
    if (col_code == 6 && is_graph)
        return (unsigned short)((is_contiguous ? 0xEE40 : 0xEE60) + row_code);
    if (col_code == 7 && is_alpha)
        {
        if (row_code == 11) return 0x00BC;
        if (row_code == 12) return 0x2016;
        if (row_code == 13) return 0x00BE;
        if (row_code == 14) return 0x00F7;
        if (row_code == 15) return 0x25A0;
        return (unsigned short)(0x70 + row_code);
        }
    if (col_code == 7 && is_graph)
        return (unsigned short)((is_contiguous ? 0xEE50 : 0xEE70) + row_code);

    return 0x20;
    }

static unsigned short utvideotex_galax_map(int row_code, int col_code,
        int is_alpha, int is_contiguous, int is_dheight,
        int is_dheight_lower)
    {
    unsigned short ch;

    if (col_code == 4 || col_code == 5)
        is_alpha = 1;
    if (row_code < 0 || row_code > 15 || col_code < 0 || col_code > 7)
        return 0x20;

    ch = 0x20;
    if (!is_alpha)
        {
        if (col_code == 2) ch = (unsigned short)(0xE200 + row_code);
        else if (col_code == 3) ch = (unsigned short)(0xE210 + row_code);
        else if (col_code == 6) ch = (unsigned short)(0xE220 + row_code);
        else if (col_code == 7) ch = (unsigned short)(0xE230 + row_code);
        if (!is_contiguous)
            ch = (unsigned short)(ch + 0xC0);
        if (is_dheight)
            ch = (unsigned short)(ch + (is_dheight_lower ? 0x80 : 0x40));
        }
    else
        {
        if (col_code == 2)
            ch = row_code == 3 ? 0x00A3 : (unsigned short)(0x20 + row_code);
        else if (col_code == 3)
            ch = (unsigned short)(0x30 + row_code);
        else if (col_code == 4)
            ch = (unsigned short)(0x40 + row_code);
        else if (col_code == 5)
            {
            if (row_code == 12) ch = 0x00BD;
            else if (row_code == 15) ch = 0x0023;
            else ch = (unsigned short)(0x50 + row_code);
            }
        else if (col_code == 6)
            ch = (unsigned short)(0x60 + row_code);
        else if (col_code == 7)
            {
            if (row_code == 11) ch = 0x00BC;
            else if (row_code == 13) ch = 0x00BE;
            else if (row_code == 14) ch = 0x00F7;
            else if (row_code == 15) ch = 0x00B6;
            else ch = (unsigned short)(0x70 + row_code);
            }
        if (is_dheight)
            ch = (unsigned short)(ch + (is_dheight_lower ? 0xE100 : 0xE000));
        }

    return ch;
    }

static void utvideotex_put_char(UTVIDTEX_RUNTIME *runtime, int row, int col,
        const UTVIDTEX_CHAR *ch, int use_upper, int use_lower,
        unsigned short flags)
    {
    unsigned short value;

    value = ch->single;
    if (use_upper)
        value = ch->upper;
    else if (use_lower)
        value = ch->lower;
    utvideotex_put_cell(runtime, row, col, value, flags);
    }

static void utvideotex_put_cell(UTVIDTEX_RUNTIME *runtime, int row, int col,
        unsigned short ch, unsigned short flags)
    {
    UTVIDTEX_CELL *cell;
    int fg;
    int bg;

    if (row < 0 || row >= UTVIDTEX_ROWS || col < 0 || col >= UTVIDTEX_COLS)
        return;

    fg = runtime->flags.is_alpha ?
        runtime->flags.alpha_fg_color : runtime->flags.mosaic_fg_color;
    bg = runtime->flags.bg_color;
    if (!utvideotex_color_is_valid(fg))
        fg = UTVIDTEX_WHITE;
    if (!utvideotex_color_is_valid(bg))
        bg = UTVIDTEX_BLACK;

    cell = &runtime->snapshot.cells[row * UTVIDTEX_COLS + col];
    cell->ch = ch;
    cell->foreground = (unsigned char)fg;
    cell->background = (unsigned char)bg;
    cell->flags = flags;
    }

static int utvideotex_copy_bytes(unsigned char *outbuf, int outcap,
        int *outlen, const unsigned char *src, int srclen)
    {
    if (outlen)
        *outlen = 0;
    if (src == 0 || srclen <= 0)
        return 0;
    if (outbuf == 0 || outcap < srclen)
        return -1;

    memcpy(outbuf, src, (size_t)srclen);
    if (outlen)
        *outlen = srclen;
    return 0;
    }

static int utvideotex_color_is_valid(int color)
    {
    return color >= UTVIDTEX_BLACK && color <= UTVIDTEX_WHITE;
    }
