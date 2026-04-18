/* Unicode-only foundation helpers for UltraTerminal-owned code. */
#if !defined(ULTRATERMINAL_UT_UNICODE_H)
#define ULTRATERMINAL_UT_UNICODE_H

#include <windows.h>

#if !defined(UNICODE) || !defined(_UNICODE)
#error UltraTerminal is now a Unicode-only Windows application.
#endif

typedef WCHAR UTCELL;

#define UT_REPLACEMENT_CHAR ((WCHAR)0xFFFD)
#define UT_WFILE (L"" __FILE__)

static __inline void ut_wstr_copy(WCHAR *pszDest, size_t cchDest,
        const WCHAR *pszSrc)
    {
    if (pszDest == 0 || cchDest == 0)
        return;

    if (pszSrc == 0)
        pszSrc = L"";

    lstrcpynW(pszDest, pszSrc, (int)cchDest);
    pszDest[cchDest - 1] = L'\0';
    }

static __inline int ut_utf8_to_wide(const char *pszSrc, WCHAR *pszDest,
        int cchDest)
    {
    int cch;

    if (pszDest == 0 || cchDest <= 0)
        return 0;

    pszDest[0] = L'\0';
    if (pszSrc == 0)
        return 0;

    cch = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszSrc, -1,
            pszDest, cchDest);
    if (cch == 0)
        cch = MultiByteToWideChar(CP_UTF8, 0, pszSrc, -1, pszDest, cchDest);

    if (cch == 0)
        {
        pszDest[0] = UT_REPLACEMENT_CHAR;
        if (cchDest > 1)
            pszDest[1] = L'\0';
        return 1;
        }

    pszDest[cchDest - 1] = L'\0';
    return cch - 1;
    }

static __inline int ut_wide_to_utf8(const WCHAR *pszSrc, char *pszDest,
        int cbDest)
    {
    int cb;

    if (pszDest == 0 || cbDest <= 0)
        return 0;

    pszDest[0] = '\0';
    if (pszSrc == 0)
        return 0;

    cb = WideCharToMultiByte(CP_UTF8, 0, pszSrc, -1, pszDest, cbDest, 0, 0);
    if (cb == 0)
        {
        if (cbDest > 1)
            {
            pszDest[0] = '?';
            pszDest[1] = '\0';
            return 1;
            }
        return 0;
        }

    pszDest[cbDest - 1] = '\0';
    return cb - 1;
    }

static __inline int ut_codepage_to_wide(UINT uCodePage, const char *pbSrc,
        int cbSrc, WCHAR *pszDest, int cchDest)
    {
    int cch;

    if (pszDest == 0 || cchDest <= 0)
        return 0;

    pszDest[0] = L'\0';
    if (pbSrc == 0 || cbSrc == 0)
        return 0;

    cch = MultiByteToWideChar(uCodePage, MB_ERR_INVALID_CHARS, pbSrc, cbSrc,
            pszDest, cchDest - 1);
    if (cch == 0)
        cch = MultiByteToWideChar(uCodePage, 0, pbSrc, cbSrc, pszDest,
                cchDest - 1);

    if (cch == 0)
        {
        pszDest[0] = UT_REPLACEMENT_CHAR;
        cch = 1;
        }

    pszDest[cch] = L'\0';
    return cch;
    }

static __inline int ut_wide_to_codepage(UINT uCodePage, const WCHAR *pszSrc,
        char *pbDest, int cbDest)
    {
    int cb;

    if (pbDest == 0 || cbDest <= 0)
        return 0;

    pbDest[0] = '\0';
    if (pszSrc == 0)
        return 0;

    cb = WideCharToMultiByte(uCodePage, 0, pszSrc, -1, pbDest, cbDest, 0, 0);
    if (cb == 0)
        return 0;

    pbDest[cbDest - 1] = '\0';
    return cb - 1;
    }

static __inline int ut_message_box_utf8(HWND hwnd, const char *pszMessage,
        const WCHAR *pszTitle, UINT uType)
    {
    WCHAR szMessage[2048];

    if (pszMessage == 0 || pszMessage[0] == '\0')
        return 0;

    if (ut_utf8_to_wide(pszMessage, szMessage,
            (int)(sizeof(szMessage) / sizeof(szMessage[0]))) == 0)
        MultiByteToWideChar(CP_ACP, 0, pszMessage, -1, szMessage,
                (int)(sizeof(szMessage) / sizeof(szMessage[0])));

    szMessage[(sizeof(szMessage) / sizeof(szMessage[0])) - 1] = L'\0';
    return MessageBoxW(hwnd, szMessage, pszTitle, uType);
    }

#endif
