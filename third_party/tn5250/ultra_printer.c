/*
 * UltraTerminal 5250 printer helper.
 *
 * The helper can run a standalone tn5250 printer session when a host is
 * supplied. UltraTerminal launches it as a sibling process and reports startup
 * failures gracefully when Wine or Windows printing is unavailable.
 */
#include "ultra_printer.h"

#include "lib5250/tn5250-private.h"

#if defined(_WIN32)
#include <windows.h>
#endif

static int find_arg(const char* cmd, const char* name, char* value,
        int value_len);
static void safe_copy(char* dst, int dst_len, const char* src);

int ut5250_printer_run(const char* command_line)
    {
    char host[256];
    char device[64];
    char map[32];
    char output[260];
    char target[320];
    Tn5250Config* config;
    Tn5250Stream* stream;
    Tn5250PrintSession* print_session;
#if defined(_WIN32)
    WSADATA wsa;
#endif

    host[0] = '\0';
    device[0] = '\0';
    map[0] = '\0';
    output[0] = '\0';
    find_arg(command_line, "--host", host, sizeof(host));
    find_arg(command_line, "--device", device, sizeof(device));
    find_arg(command_line, "--map", map, sizeof(map));
    find_arg(command_line, "--output", output, sizeof(output));

    if (host[0] == '\0')
        {
#if defined(_WIN32)
        MessageBoxW(NULL,
                L"UltraTerminal5250Printer.exe started without a host. "
                L"Start it from an active IBM 5250 session with printer "
                L"settings that include a host.",
                L"UltraTerminal 5250 Printer", MB_OK | MB_ICONINFORMATION);
#endif
        return 0;
        }

#if defined(_WIN32)
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 2;
#endif

    config = tn5250_config_new();
    if (config == NULL)
        return 3;

    tn5250_config_set(config, "env.TERM", "IBM-3812-1");
    if (device[0])
        tn5250_config_set(config, "env.DEVNAME", device);
    tn5250_config_set(config, "map", map[0] ? map : "37");

    if (strstr(host, ":") == NULL && strstr(host, "tn5250:") != host &&
            strstr(host, "telnet:") != host)
        {
        snprintf(target, sizeof(target), "tn5250:%s", host);
        target[sizeof(target) - 1] = '\0';
        }
    else
        {
        safe_copy(target, sizeof(target), host);
        }

    stream = tn5250_stream_open(target, config);
    if (stream == NULL)
        {
        tn5250_config_unref(config);
#if defined(_WIN32)
        WSACleanup();
#endif
        return 4;
        }

    print_session = tn5250_print_session_new();
    if (print_session == NULL)
        {
        tn5250_stream_destroy(stream);
        tn5250_config_unref(config);
#if defined(_WIN32)
        WSACleanup();
#endif
        return 5;
        }

    tn5250_print_session_set_stream(print_session, stream);
    tn5250_print_session_set_char_map(print_session, map[0] ? map : "37");
    if (output[0])
        tn5250_print_session_set_output_command(print_session, output);
    tn5250_print_session_main_loop(print_session);

    tn5250_print_session_destroy(print_session);
    tn5250_config_unref(config);
#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
    }

static int find_arg(const char* cmd, const char* name, char* value,
        int value_len)
    {
    const char* pos;
    const char* start;
    const char* end;
    int len;

    if (cmd == NULL || name == NULL || value == NULL || value_len <= 0)
        return 0;

    value[0] = '\0';
    pos = strstr(cmd, name);
    if (pos == NULL)
        return 0;

    start = pos + strlen(name);
    while (*start == ' ' || *start == '\t')
        ++start;
    if (*start == '"')
        {
        ++start;
        end = strchr(start, '"');
        }
    else
        {
        end = start;
        while (*end && *end != ' ' && *end != '\t')
            ++end;
        }
    if (end == NULL)
        end = start + strlen(start);

    len = (int)(end - start);
    if (len >= value_len)
        len = value_len - 1;
    if (len < 0)
        len = 0;
    memcpy(value, start, len);
    value[len] = '\0';
    return len > 0;
    }

static void safe_copy(char* dst, int dst_len, const char* src)
    {
    if (dst == NULL || dst_len <= 0)
        return;
    if (src == NULL)
        src = "";
    strncpy(dst, src, dst_len - 1);
    dst[dst_len - 1] = '\0';
    }
