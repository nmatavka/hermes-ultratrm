#ifndef ULTRATERMINAL_HTMLHELP_H
#define ULTRATERMINAL_HTMLHELP_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HH_DISPLAY_TOPIC 0x0000
#define HH_HELP_FINDER HH_DISPLAY_TOPIC

HWND WINAPI HtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);
HWND WINAPI HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);

#ifdef UNICODE
#define HtmlHelp HtmlHelpW
#else
#define HtmlHelp HtmlHelpA
#endif

#ifdef __cplusplus
}
#endif

#endif
