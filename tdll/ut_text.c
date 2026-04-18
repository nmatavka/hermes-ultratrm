/* Unicode-only string helpers for UltraTerminal. */

#include <windows.h>
#pragma hdrstop

#include <string.h>

#include "stdtyp.h"
#include "tdll.h"
#include "assert.h"
#include "ut_text.h"
#include "ut_unicode.h"

static int ut_utf8_expected_len(UCHAR ch)
	{
	if ((ch & 0x80) == 0)
		return 1;
	if ((ch & 0xE0) == 0xC0)
		return 2;
	if ((ch & 0xF0) == 0xE0)
		return 3;
	if ((ch & 0xF8) == 0xF0)
		return 4;
	return 1;
	}

WCHAR *WCHAR_Fill(WCHAR *dest, WCHAR c, size_t count)
	{
	size_t i;

	if (dest == NULL)
		return NULL;

	for (i = 0; i < count; ++i)
		dest[i] = c;

	return dest;
	}

WCHAR *WCHAR_Trim(WCHAR *pszStr)
	{
	WCHAR *pszStart;
	WCHAR *pszEnd;
	size_t cch;

	if (pszStr == NULL)
		return NULL;

	pszStart = pszStr;
	while (*pszStart == L' ' || *pszStart == L'\t' ||
			*pszStart == L'\n' || *pszStart == L'\v' ||
			*pszStart == L'\f' || *pszStart == L'\r')
		++pszStart;

	if (pszStart != pszStr)
		memmove(pszStr, pszStart, (lstrlenW(pszStart) + 1) * sizeof(WCHAR));

	cch = (size_t)lstrlenW(pszStr);
	while (cch > 0)
		{
		pszEnd = pszStr + cch - 1;
		if (*pszEnd != L' ' && *pszEnd != L'\t' &&
				*pszEnd != L'\n' && *pszEnd != L'\v' &&
				*pszEnd != L'\f' && *pszEnd != L'\r')
			break;
		*pszEnd = L'\0';
		--cch;
		}

	return pszStr;
	}

LPWSTR StrCharNext(LPCWSTR pszStr)
	{
	return pszStr ? (LPWSTR)(pszStr + 1) : NULL;
	}

LPWSTR StrCharPrev(LPCWSTR pszStart, LPCWSTR pszStr)
	{
	if (pszStart == NULL || pszStr == NULL)
		return NULL;

	return (LPWSTR)(pszStr > pszStart ? pszStr - 1 : pszStart);
	}

LPWSTR StrCharLast(LPCWSTR pszStr)
	{
	int cch;

	if (pszStr == NULL)
		return NULL;

	cch = lstrlenW(pszStr);
	return (LPWSTR)(cch > 0 ? pszStr + cch - 1 : pszStr);
	}

LPWSTR StrCharEnd(LPCWSTR pszStr)
	{
	return pszStr ? (LPWSTR)(pszStr + lstrlenW(pszStr)) : NULL;
	}

LPWSTR StrCharFindFirst(LPCWSTR pszStr, int nChar)
	{
	if (pszStr == NULL)
		return NULL;

	for (; *pszStr != L'\0'; ++pszStr)
		{
		if (*pszStr == (WCHAR)nChar)
			return (LPWSTR)pszStr;
		}

	return NULL;
	}

LPWSTR StrCharFindLast(LPCWSTR pszStr, int nChar)
	{
	LPCWSTR pszFound = NULL;

	if (pszStr == NULL)
		return NULL;

	for (; *pszStr != L'\0'; ++pszStr)
		{
		if (*pszStr == (WCHAR)nChar)
			pszFound = pszStr;
		}

	return (LPWSTR)pszFound;
	}

LPWSTR StrCharCopy(LPWSTR pszDst, LPCWSTR pszSrc)
	{
	return lstrcpyW(pszDst, pszSrc ? pszSrc : L"");
	}

LPWSTR StrCharCat(LPWSTR pszDst, LPCWSTR pszSrc)
	{
	return lstrcatW(pszDst, pszSrc ? pszSrc : L"");
	}

LPWSTR StrCharStrStr(LPCWSTR pszA, LPCWSTR pszB)
	{
	if (pszA == NULL || pszB == NULL)
		return NULL;
	return wcsstr(pszA, pszB);
	}

LPWSTR StrCharCopyN(LPWSTR pszDst, LPCWSTR pszSrc, int iLen)
	{
	if (pszDst == NULL || iLen <= 0)
		return pszDst;

	lstrcpynW(pszDst, pszSrc ? pszSrc : L"", iLen);
	pszDst[iLen - 1] = L'\0';
	return pszDst;
	}

int StrCharGetStrLength(LPCWSTR pszStr)
	{
	return pszStr ? lstrlenW(pszStr) : 0;
	}

int StrCharGetByteCount(LPCWSTR pszStr)
	{
	return pszStr ? (int)(lstrlenW(pszStr) * sizeof(WCHAR)) : 0;
	}

int StrCharCmp(LPCWSTR pszA, LPCWSTR pszB)
	{
	return lstrcmpW(pszA ? pszA : L"", pszB ? pszB : L"");
	}

int StrCharCmpi(LPCWSTR pszA, LPCWSTR pszB)
	{
	return lstrcmpiW(pszA ? pszA : L"", pszB ? pszB : L"");
	}

ECHAR *ECHAR_Fill(ECHAR *dest, ECHAR c, size_t count)
	{
	size_t i;

	if (dest == NULL)
		return NULL;

	for (i = 0; i < count; ++i)
		dest[i] = c;

	return dest;
	}

int CnvrtHostToECHAR(ECHAR *echrDest, const unsigned long ulDestSize,
		const WCHAR * const pszSource, const unsigned long ulSourceSize)
	{
	unsigned long cchDest;
	unsigned long cchSource;
	unsigned long cchCopy;

	if (echrDest == NULL || ulDestSize < sizeof(ECHAR))
		return 0;

	cchDest = ulDestSize / sizeof(ECHAR);
	cchSource = pszSource ? ulSourceSize / sizeof(WCHAR) : 0;
	if (pszSource != NULL)
		{
		unsigned long cchString = (unsigned long)lstrlenW(pszSource) + 1;
		if (cchSource == 0 || cchSource > cchString)
			cchSource = cchString;
		}

	cchCopy = cchSource;
	if (cchCopy >= cchDest)
		cchCopy = cchDest - 1;

	if (pszSource != NULL && cchCopy > 0)
		memcpy(echrDest, pszSource, cchCopy * sizeof(ECHAR));
	echrDest[cchCopy] = ECHAR_CAST('\0');

	return (int)(cchCopy * sizeof(ECHAR));
	}

int CnvrtECHARtoHost(WCHAR *pszDest, const unsigned long ulDestSize,
		const ECHAR * const echrSource, const unsigned long ulSourceSize)
	{
	unsigned long cchDest;
	unsigned long cchSource;
	unsigned long cchCopy;

	if (pszDest == NULL || ulDestSize < sizeof(WCHAR))
		return 0;

	cchDest = ulDestSize / sizeof(WCHAR);
	cchSource = echrSource ? ulSourceSize / sizeof(ECHAR) : 0;
	if (echrSource != NULL)
		{
		unsigned long cchString = (unsigned long)StrCharGetEcharLen(echrSource) + 1;
		if (cchSource == 0 || cchSource > cchString)
			cchSource = cchString;
		}

	cchCopy = cchSource;
	if (cchCopy >= cchDest)
		cchCopy = cchDest - 1;

	if (echrSource != NULL && cchCopy > 0)
		memcpy(pszDest, echrSource, cchCopy * sizeof(WCHAR));
	pszDest[cchCopy] = L'\0';

	return (int)(cchCopy * sizeof(WCHAR));
	}

int CnvrtECHARtoWCHAR(LPWSTR pszDest, int cchDest, ECHAR eChar)
	{
	if (pszDest == NULL || cchDest <= 0)
		return 0;

	pszDest[0] = (WCHAR)eChar;
	if (cchDest > 1)
		pszDest[1] = L'\0';
	return 1;
	}

int StrCharGetEcharLen(const ECHAR * const pszA)
	{
	int cch = 0;

	if (pszA == NULL)
		return 0;

	while (pszA[cch] != ECHAR_CAST('\0'))
		++cch;

	return cch;
	}

int StrCharGetEcharByteCount(const ECHAR * const pszA)
	{
	return StrCharGetEcharLen(pszA) * (int)sizeof(ECHAR);
	}

int StrCharCmpEtoT(const ECHAR * const pszA, const WCHAR * const pszB)
	{
	const ECHAR *pa = pszA ? pszA : L"";
	const WCHAR *pb = pszB ? pszB : L"";

	while (*pa != ECHAR_CAST('\0') && *pb != L'\0' && *pa == (ECHAR)*pb)
		{
		++pa;
		++pb;
		}

	return (int)*pa - (int)*pb;
	}

int isHostWideChar(unsigned int Char)
	{
	(void)Char;
	return FALSE;
	}

int StrCharStripHostWideString(ECHAR *aechDest, const long lDestSize,
		ECHAR *aechSource)
	{
	long cchDest;
	long cchCopy;

	if (aechDest == NULL || lDestSize <= 0)
		return 0;

	cchDest = lDestSize / (long)sizeof(ECHAR);
	if (cchDest <= 0)
		return 0;

	cchCopy = aechSource ? (long)StrCharGetEcharLen(aechSource) : 0;
	if (cchCopy >= cchDest)
		cchCopy = cchDest - 1;

	if (aechSource != NULL && cchCopy > 0)
		memcpy(aechDest, aechSource, (size_t)cchCopy * sizeof(ECHAR));
	aechDest[cchCopy] = ECHAR_CAST('\0');
	return 0;
	}

#if defined(INCL_VTUTF8)
BOOLEAN TranslateUTF8ToHost(UCHAR IncomingByte, UCHAR *pUTF8Buffer,
		int iUTF8BufferLength, WCHAR *pUnicodeBuffer,
		int iUnicodeBufferLength, WCHAR *pHostBuffer, int iHostBufferLength)
	{
	if (pUTF8Buffer == NULL || iUTF8BufferLength <= 1 ||
			pUnicodeBuffer == NULL || iUnicodeBufferLength <= 0 ||
			pHostBuffer == NULL || iHostBufferLength <= 0)
		return FALSE;

	if (TranslateUtf8ToUnicode(IncomingByte, pUTF8Buffer, pUnicodeBuffer))
		{
		pHostBuffer[0] = pUnicodeBuffer[0];
		if (iHostBufferLength > 1)
			pHostBuffer[1] = L'\0';
		return TRUE;
		}

	return FALSE;
	}

BOOLEAN TranslateHostToUTF8(const WCHAR *pHostBuffer, int iHostBufferLength,
		WCHAR *pUnicodeBuffer, int iUnicodeBufferLength, UCHAR *pUTF8Buffer,
		int iUTF8BufferLength)
	{
	(void)iHostBufferLength;

	if (pHostBuffer == NULL || pUnicodeBuffer == NULL ||
			iUnicodeBufferLength <= 0 || pUTF8Buffer == NULL ||
			iUTF8BufferLength <= 0)
		return FALSE;

	pUnicodeBuffer[0] = pHostBuffer[0];
	if (iUnicodeBufferLength > 1)
		pUnicodeBuffer[1] = L'\0';

	return TranslateUnicodeToUtf8(pUnicodeBuffer, pUTF8Buffer);
	}

BOOLEAN TranslateUnicodeToUtf8(PCWSTR SourceBuffer, UCHAR *DestinationBuffer)
	{
	int cb;

	if (SourceBuffer == NULL || DestinationBuffer == NULL)
		return FALSE;

	cb = WideCharToMultiByte(CP_UTF8, 0, SourceBuffer, -1,
			(char *)DestinationBuffer, 8, NULL, NULL);
	return cb > 0;
	}

BOOLEAN TranslateUtf8ToUnicode(UCHAR IncomingByte, UCHAR *ExistingUtf8Buffer,
		WCHAR *DestinationUnicodeVal)
	{
	size_t cb;
	int expected;
	int cch;

	if (ExistingUtf8Buffer == NULL || DestinationUnicodeVal == NULL)
		return FALSE;

	cb = strlen((const char *)ExistingUtf8Buffer);
	if (cb >= 7)
		cb = 0;
	ExistingUtf8Buffer[cb++] = IncomingByte;
	ExistingUtf8Buffer[cb] = '\0';

	expected = ut_utf8_expected_len(ExistingUtf8Buffer[0]);
	if ((int)cb < expected)
		return FALSE;

	cch = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			(const char *)ExistingUtf8Buffer, (int)cb,
			DestinationUnicodeVal, 2);
	if (cch <= 0)
		{
		DestinationUnicodeVal[0] = UT_REPLACEMENT_CHAR;
		DestinationUnicodeVal[1] = L'\0';
		ExistingUtf8Buffer[0] = '\0';
		return TRUE;
		}

	DestinationUnicodeVal[cch] = L'\0';
	ExistingUtf8Buffer[0] = '\0';
	return TRUE;
	}
#endif
