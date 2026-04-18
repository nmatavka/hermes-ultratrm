/* Unicode-only string helper declarations. */

#if !defined(ULTRATERMINAL_UT_TEXT_H)
#define ULTRATERMINAL_UT_TEXT_H

WCHAR 	*WCHAR_Fill(WCHAR *dest, WCHAR c, size_t count);
WCHAR 	*WCHAR_Trim(WCHAR *pszStr);
LPWSTR 	StrCharNext(LPCWSTR pszStr);
LPWSTR 	StrCharPrev(LPCWSTR pszStart, LPCWSTR pszStr);
LPWSTR 	StrCharLast(LPCWSTR pszStr);
LPWSTR 	StrCharEnd(LPCWSTR pszStr);
LPWSTR 	StrCharFindFirst(LPCWSTR pszStr, int nChar);
LPWSTR 	StrCharFindLast(LPCWSTR pszStr, int nChar);
LPWSTR 	StrCharCopy(LPWSTR pszDst, LPCWSTR pszSrc);
LPWSTR 	StrCharCat(LPWSTR pszDst, LPCWSTR pszSrc);
LPWSTR 	StrCharStrStr(LPCWSTR pszA, LPCWSTR pszB);
LPWSTR 	StrCharCopyN(LPWSTR pszDst, LPCWSTR pszSrc, int iLen);
int 	StrCharGetStrLength(LPCWSTR pszStr);
int 	StrCharGetByteCount(LPCWSTR pszStr);
int 	StrCharCmp(LPCWSTR pszA, LPCWSTR pszB);
int 	StrCharCmpi(LPCWSTR pszA, LPCWSTR pszB);

ECHAR 	*ECHAR_Fill(ECHAR *dest, ECHAR c, size_t count);
int 	CnvrtHostToECHAR(ECHAR *echrDest, const unsigned long ulDestSize,
			const WCHAR * const pszSource, const unsigned long ulSourceSize);
int 	CnvrtECHARtoHost(WCHAR *pszDest, const unsigned long ulDestSize,
			const ECHAR * const echrSource, const unsigned long ulSourceSize);
int 	CnvrtECHARtoWCHAR(LPWSTR pszDest, int cchDest, ECHAR eChar);
int 	StrCharGetEcharLen(const ECHAR * const pszA);
int 	StrCharGetEcharByteCount(const ECHAR * const pszA);
int 	StrCharCmpEtoT(const ECHAR * const pszA, const WCHAR * const pszB);
int 	StrCharStripHostWideString(ECHAR *aechDest, const long lDestSize,
            ECHAR *aechSource);
int 	isHostWideChar(unsigned int Char);

#if defined(INCL_VTUTF8)
BOOLEAN TranslateUTF8ToHost(UCHAR IncomingByte, UCHAR *pUTF8Buffer,
                            int iUTF8BufferLength, WCHAR *pUnicodeBuffer,
                            int iUnicodeBufferLength, WCHAR *pHostBuffer,
                            int iHostBufferLength);
BOOLEAN TranslateHostToUTF8(const WCHAR *pHostBuffer, int iHostBufferLength,
                                  WCHAR *pUnicodeBuffer,
                                  int iUnicodeBufferLength,
                                  UCHAR *pUTF8Buffer,
                                  int iUTF8BufferLength);
BOOLEAN TranslateUnicodeToUtf8(PCWSTR SourceBuffer,
                               UCHAR  *DestinationBuffer);
BOOLEAN TranslateUtf8ToUnicode(UCHAR IncomingByte,
                               UCHAR *ExistingUtf8Buffer,
                               WCHAR *DestinationUnicodeVal);
#endif

#endif
