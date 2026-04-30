/*
 * File:        open_msc.c - stuff for calling Common Open Dialog
 *
 * Copyright 1991 by Hilgraeve Inc. -- Monroe, MI
 * All rights reserved
 *
 * $Revision: 11 $
 * $Date: 5/01/01 1:45p $
 */

#include <windows.h>
#pragma hdrstop

// #define      DEBUGSTR        1
#include <commdlg.h>
#include <memory.h>
#include <stdlib.h>
#include <shlobj.h>
#include <tdll/stdtyp.h>
#include <term/res.h>
#include <tdll/mc.h>
#include <tdll/tdll.h>
#include <tdll/globals.h>
#include <tdll/file_msc.h>
#include <tdll/load_res.h>
#include <tdll/ut_text.h>
#include <tdll/assert.h>
#include <tdll/misc.h>
#include "registry.h"
#include "open_msc.h"

static OPENFILENAME ofn;
static BROWSEINFO bi;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
// This function prototype changed from BOOL FAR PASCAL. -- REV 3-6-98.
UINT_PTR APIENTRY gnrcFindDirHookProc(HWND hdlg,
		    						  UINT msg,
			    					  WPARAM wPar,
				    				  LPARAM lPar)
	{
   	WCHAR   acMsg[64];
	WORD    windowID;

	windowID = LOWORD(wPar);

	switch (msg)
		{
		case WM_INITDIALOG:
			break;

		case WM_DESTROY:
			break;

		case WM_COMMAND:
			switch (windowID)
				{
				case IDOK:
            		LoadString(glblQueryDllHinst(),
			        	        40809,
				                acMsg,
				                sizeof(acMsg) / sizeof(WCHAR));
					SetDlgItemText(hdlg, edt1, acMsg);

					// EndDialog(hdlg, 1);
					break;

				case lst2:
					if (HIWORD(lPar) == LBN_DBLCLK)
						{
						SetFocus(GetDlgItem(hdlg, IDOK));
						PostMessage(hdlg,
									WM_COMMAND,
									IDOK,
									MAKELONG((INT_PTR)GetDlgItem(hdlg, IDOK),0));
						}
					break;

				default:
					break;
				}
			break;

		default:
			break;
		}

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
#if FALSE
BOOL FAR PASCAL gnrcFindFileHookProc(HWND hdlg,
									UINT msg,
									WPARAM wPar,
									LPARAM lPar)
	{
	WORD    windowID;

	windowID = LOWORD(wPar);

	switch (msg)
		{
		case WM_INITDIALOG:
			ofn.lCustData = 0;
			break;

		case WM_DESTROY:
			break;

		default:
			break;
		}

	return FALSE;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      gnrcFindFileDialog
 *
 * DESCRIPTION:
 *      This function makes the FindFile common dialog a little bit easier to
 *      call.
 *
 * ARGUEMENTS:
 *      hwnd            -- the window handle to use as the parent
 *      pszTitle        -- text to display as the title
 *      pszDirectory    -- path to use as default directory
 *      pszMasks        -- file name masks
 *
 * RETURNS:
 *      A pointer to a string that contains the file name.  This string is malloced
 *      and must be freed by the caller, or..
 *      NULL which indicates the user canceled the operation.
 */

LPWSTR gnrcFindFileDialogInternal(HWND hwnd,
								LPCWSTR pszTitle,
								LPCWSTR pszDirectory,
								LPCWSTR pszMasks,
								int nFindFlag,
								LPCWSTR pszInitName)
	{
	int index;
	LPWSTR  pszRet = NULL;
	LPWSTR  pszStr;
	LPCWSTR pszWrk;
	WCHAR   acMask[128];
	WCHAR   acTitle[64];
	WCHAR   szExt[4];
	WCHAR   szFile[FNAME_LEN + 1];
	WCHAR   szDir[FNAME_LEN + 1];
	int     iRet;
    int     iSize;
	int     iExtSize;
	//DWORD   dwMaxComponentLength;
	//DWORD   dwFileSystemFlags;

	memset((LPWSTR)&ofn, 0, sizeof(ofn));
	WCHAR_Fill(szFile, L'\0', sizeof(szFile)/sizeof(WCHAR));
	WCHAR_Fill(szExt, L'\0', sizeof(szExt)/sizeof(WCHAR));
	WCHAR_Fill(acMask, L'\0', sizeof(acMask)/sizeof(WCHAR));
	WCHAR_Fill(acTitle, L'\0', sizeof(acTitle)/sizeof(WCHAR));

    ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner             = hwnd;
	ofn.hInstance             = (HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	if ((pszMasks == NULL) || (StrCharGetStrLength(pszMasks) == 0))
		{
		resLoadFileMask(glblQueryDllHinst(),
						IDS_CMM_ALL_FILES1,
						1,
						acMask,
						sizeof(acMask) / sizeof(WCHAR));

		ofn.lpstrFilter = acMask;
		}
	else
		{
		ofn.lpstrFilter   = pszMasks;
		pszWrk = pszMasks;
		pszWrk = StrCharFindFirst(pszWrk, L'.');

		if (*pszWrk == '.')
			{
			pszWrk = StrCharNext(pszWrk);
			pszStr = (LPWSTR)pszWrk;
			index = 0;
			/* This works because we know how the mask are going to be formed */
			while ((index < 3) && (*pszWrk != ')'))
				{
				index += 1;
				pszWrk = StrCharNext(pszWrk);
				}
			if (pszWrk >= pszStr)
				MemCopy(szExt, pszStr, pszWrk - pszStr);
			}

		pszWrk = NULL;
		}

    iSize = StrCharGetByteCount(pszInitName);
    //MPT:10SEP98 if there is no name, just set the dest string to nothing
	if (pszInitName && iSize > 0 && iSize < (int)sizeof(szFile))
		{
		MemCopy(szFile, pszInitName, (size_t)iSize);
		}
    else
        {
        szFile[0] = L'\0';
        }

	ofn.lpstrDefExt       = (LPWSTR)szExt;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = (LPWSTR)szFile;
	ofn.nMaxFile          = FNAME_LEN;

	if ((pszDirectory == NULL) || (StrCharGetStrLength(pszDirectory) == 0))
		{
	
#ifdef NT_EDITION
		// mpt:07-30-97
		if (IsNT())
#endif
			GetUserDirectory(szDir, sizeof(szDir) / sizeof(WCHAR));
#ifdef NT_EDITION
		else
			{
			WCHAR acDirTmp[FNAME_LEN];
			GetModuleFileName(glblQueryHinst(), acDirTmp, sizeof(acDirTmp));
			mscStripName(acDirTmp);
			}
#endif
		}

	else
		{
		StrCharCopy(szDir, pszDirectory);
		}

	pszStr = StrCharLast(szDir);
	if (*pszStr == L'\\')
		*pszStr = L'\0';

	ofn.lpstrInitialDir   = szDir;
	ofn.lpstrFileTitle        = NULL;
	ofn.nMaxFileTitle         = 0;

	if ((pszTitle == NULL) || (StrCharGetByteCount(pszTitle) == 0))
		{
		// ofn.lpstrTitle         = "Select File";
		LoadString(glblQueryDllHinst(),
				IDS_CPF_SELECT_FILE,
				acTitle,
				sizeof(acTitle) / sizeof(WCHAR));

		ofn.lpstrTitle = acTitle;
		}

	else
		{
		ofn.lpstrTitle = pszTitle;
		}

	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	//ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

	if (nFindFlag)
		{
		ofn.Flags |= OFN_FILEMUSTEXIST;
		iRet = GetOpenFileName(&ofn);
		}

	else
		{
		#if 0
		// This was added so the common dialog for "Save As" applies
		// the same restrictions as the New Connect dialog when it
		// comes to saving session file names.
		//
		GetVolumeInformation(NULL, NULL, 0, NULL, &dwMaxComponentLength,
								 &dwFileSystemFlags, NULL, 0);

		if(dwMaxComponentLength == 255)
			{
			ofn.nMaxFile = dwMaxComponentLength - 1;
			}
		else
			{
			ofn.nMaxFile = 8;
			}
		#endif

		iRet = GetSaveFileName(&ofn);
		}


	if (iRet == TRUE)
		{
		iExtSize = StrCharGetStrLength(ofn.lpstrDefExt) + StrCharGetStrLength(L".");
		iSize = min(StrCharGetStrLength(ofn.lpstrFile) + iExtSize, (int)ofn.nMaxFile);

		if (iSize > 0)
			{
			size_t const cchRet = (size_t)ofn.nMaxFile + 1;
			pszRet = malloc(cchRet * sizeof(WCHAR));

			if (pszRet != NULL)
				{
				WCHAR_Fill(pszRet, L'\0', cchRet);

				// Due to a bug in GetSaveFileName(), it is possible
				// the file will not contain the default extension if
				// the filename is too long for the default extension
				// to be appended.  We need to make sure we have the
				// proper file extension type.  REV: 10/18/2000
				//
				if(ofn.nFileExtension == 0 && nFindFlag == FALSE)
					{
					StrCharCopyN(pszRet, ofn.lpstrFile, iSize - iExtSize);
					StrCharCat(pszRet, ".");
					StrCharCat(pszRet, ofn.lpstrDefExt);
					}
				else
					{
					StrCharCopy(pszRet, ofn.lpstrFile);
					}

				// make sure this is a NULL terminated string.
				//
				pszRet[cchRet - 1] = L'\0';
				}

			return pszRet;
			}
		else
			{
			return(NULL);
			}
		}

	else
		{
		return(NULL);
		}

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
LPWSTR gnrcFindFileDialog(HWND hwnd,
						LPCWSTR pszTitle,
						LPCWSTR pszDirectory,
						LPCWSTR pszMasks)
	{
	return gnrcFindFileDialogInternal(hwnd,
									pszTitle,
									pszDirectory,
									pszMasks,
									TRUE,
									NULL);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
LPWSTR gnrcSaveFileDialog(HWND hwnd,
						LPCWSTR pszTitle,
						LPCWSTR pszDirectory,
						LPCWSTR pszMasks,
						LPCWSTR pszInitName)
	{
	return gnrcFindFileDialogInternal(hwnd,
									pszTitle,
									pszDirectory,
									pszMasks,
									FALSE,
									pszInitName);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  gnrcFindDirectoryDialog
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
LPWSTR gnrcFindDirectoryDialog(HWND hwnd, HSESSION hSession, LPWSTR pszDir)
	{
#ifndef INCL_USE_NEWFOLDERDLG
    BOOL bRet;
#else
    LPITEMIDLIST pidlRoot = 0, pidlSelected = 0;
    LPSHELLFOLDER lpDesktop = NULL;
    ULONG chEaten = 0;
#endif
	LPWSTR pszStr;
	WCHAR acTitle[64];
	WCHAR acList[64];
	WCHAR szDir[FNAME_LEN];
	WCHAR szFile[FNAME_LEN];

	LoadString(glblQueryDllHinst(),
			IDS_CMM_SEL_DIR,
			acTitle,
			sizeof(acTitle) / sizeof(WCHAR));

	resLoadFileMask(glblQueryDllHinst(),
				IDS_CMM_ALL_FILES1,
				1,
				acList,
				sizeof(acList) / sizeof(WCHAR));

	WCHAR_Fill(szFile, L'\0', sizeof(szFile) / sizeof(WCHAR));
	memset((LPWSTR)&ofn, 0, sizeof(ofn));

	if ((pszDir == NULL) || (StrCharGetStrLength(pszDir) == 0))
		{

		//changed to use working folder rather than current - mpt 8-18-99
		if ( !GetWorkingDirectory(szDir, FNAME_LEN) )
			{
			GetCurrentDirectory(FNAME_LEN, szDir);
			}
		}
	else
		{
		StrCharCopy(szDir, pszDir);
		}

	pszStr = StrCharLast(szDir);
	if (*pszStr == L'\\')
		*pszStr = L'\0';

#ifndef INCL_USE_NEWFOLDERDLG
    ofn.lCustData         = 0L;
    ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner         = hwnd;
	ofn.hInstance         = (HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	ofn.lpstrTitle        = acTitle;
	ofn.lpstrFilter       = acList;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = szFile;
	ofn.nMaxFile          = sizeof(szFile);
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lpstrFileTitle    = acTitle;
	ofn.nMaxFileTitle     = sizeof(acTitle);
	ofn.lpstrDefExt       = NULL;
	// If OFN_ENABLEHOOK and/or OFN_ENABLETEMPLATE flags are set the call to
	// GetOpenFileName() will crash the application, then only a single drive
	// will appear in the "Drives:" dropdown list.  This is a bug in the 1691
	// build of Windows 98. -- REV 3-6-98.
	//
	ofn.Flags             = OFN_ENABLEHOOK | OFN_ENABLETEMPLATE |
							OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

	ofn.lpfnHook          = gnrcFindDirHookProc;
	ofn.lpTemplateName    = MAKEINTRESOURCE(IDD_FINDDIRECTORY);

	ofn.lpstrInitialDir   = szDir;

	bRet = GetOpenFileName(&ofn);

    if (StrCharGetStrLength(szFile) == 0)
		return NULL;

	pszStr = StrCharFindLast(szFile, L'\\');
	if (*pszStr == L'\\')
		{
		pszStr = StrCharNext(pszStr);
		*pszStr = L'\0';
		}

#else //INCL_USE_NEWFOLDERDLG

    //TODO:MPT Finish up the new browse for folder mechanism
    //         - Initialize pidlRoot with szDir
    //         - free pidl's when done
    //         - Use callback proc to verify the folder is writable

    CoInitialize( 0 );
    
    bi.pidlRoot = pidlRoot;
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = szDir;
    bi.lpszTitle = acTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    bi.lpfn = NULL; //gnrcBrowseFolderHookProc;
    bi.lParam = 0;
    
    pidlSelected = SHBrowseForFolder( &bi );
    SHGetPathFromIDList( pidlSelected, szFile );

    if (StrCharGetStrLength(szFile) == 0)
		return NULL;

#endif	

	fileFinalizeDIR(hSession, szFile, szFile);

	pszStr = malloc((size_t)StrCharGetByteCount(szFile) + sizeof(WCHAR));
	StrCharCopy(pszStr, szFile);

	return pszStr;
	}

#ifdef INCL_USE_NEWFOLDERDLG
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  gnrcBrowseFolderHookProc
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
UINT_PTR CALLBACK gnrcBrowseFolderHookProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {
    return 0;
    }
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  GetUserDirectory
 *
 * DESCRIPTION:
 *  Returns the default UltraTerminal directory for the current user
 *
 * ARGUMENTS:
 *  pszUserDir  --  where to write the default directory
 *  dwSize      --  size, in characters, of the above buffer
 *
 * RETURNS:
 *  If the function succeeds, the return value is the number of characters
 *  stored into the buffer pointed to by pszDir, not including the
 *  terminating null character.
 *
 *  If the specified environment variable name was not found in the
 *  environment block for the current process, the return value is zero.
 *
 *  If the buffer pointed to by lpBuffer is not large enough, the return
 *  value is the buffer size, in characters, required to hold the value
 *  string and its terminating null character.
 *
 * Author:  JMH, 6-12-96
 */
DWORD GetUserDirectory(LPWSTR pszUserDir, DWORD dwSize)
    {
    DWORD   dwRet = MAX_PATH;
    WCHAR   szProfileDir[_MAX_PATH * sizeof(WCHAR)];
    WCHAR   szProfileDir1[_MAX_PATH * sizeof(WCHAR)];
    WCHAR   szProfileRoot[_MAX_PATH * sizeof(WCHAR)];


    szProfileRoot[0] = L'\0';
    if ( regQueryValue(HKEY_CURRENT_USER,
                       L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                       L"Programs",
                       szProfileRoot,
                       &dwRet) != 0 )
        {
        assert(0);
        return 0;
        }

    dwRet = MAX_PATH;
    szProfileDir[0] = L'\0';

    if ( regQueryValue(HKEY_CURRENT_USER,
                       L"SOFTWARE\\Hilgraeve Inc\\UltraTerminal PE\\3.0",
                       L"SessionsPath",
                       szProfileDir,
                       &dwRet) != 0 )
        {
        dwRet = MAX_PATH;
        szProfileDir[0] = L'\0';
        if ( regQueryValue(HKEY_CURRENT_USER,
                           L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GrpConv\\MapGroups",
                           L"Communications",
                           szProfileDir,
                           &dwRet) != 0 )
            {
            //mpt:12-16-98 If UltraTerminal was never installed by the operating system, this
            //             key is not present so we need to fake one.
            LoadString(glblQueryDllHinst(), IDS_GNRL_PROFILE_DIR, szProfileDir, sizeof(szProfileDir)/sizeof(WCHAR) );
            }

        szProfileDir1[0] = L'\0';
        LoadString(glblQueryDllHinst(), IDS_GNRL_APPNAME, szProfileDir1, sizeof(szProfileDir)/sizeof(WCHAR) );



        // Bug #440648, code is not functioning correctly for legacy double-byte.
        //              Fix is to remove '\' from IDS_GNRL_PROFILE_DIR
        //              and always append '\' below.
        //
        //if (*szProfileDir &&
        //    L'\\' != szProfileDir[StrCharGetStrLength(szProfileDir)-1])


        lstrcatW( szProfileDir, L"\\" );

        lstrcatW( szProfileDir, szProfileDir1 );


        dwRet = StrCharGetStrLength(szProfileRoot) + StrCharGetStrLength(szProfileDir);
        if (dwRet + sizeof(WCHAR) < dwSize)
            {
            wsprintf(pszUserDir, L"%s\\%s", szProfileRoot, szProfileDir);
            }
        }
    else
        {
        dwRet = StrCharGetStrLength(szProfileDir);
        if (dwRet + sizeof(WCHAR) < dwSize)
            {
            wsprintf(pszUserDir, L"%s", szProfileDir);
            }
        }

    return dwRet;
    }


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  CreateUserDirectory (NT_EDITION only)
 *
 * DESCRIPTION:
 *  For NT, if HT is installed after the os update, no directory
 *  will exist for HT in the user profile. This function creates the
 *  directory if necessary, since the rest of the program assumes
 *  it exists.
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  Nothing.
 *
 * Author:  JMH, 6-13-96
 */
void CreateUserDirectory(void)
    {
    WCHAR   szUserDir[_MAX_PATH];

    GetUserDirectory(szUserDir, sizeof(szUserDir) / sizeof(WCHAR));
    mscCreatePath(szUserDir);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  IsNT
 *
 * DESCRIPTION: Determines if we are running under Windows NT
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  True if NT
 *
 * Author:  MPT 7-31-97
 */
BOOL IsNT(void)
	{

	OSVERSIONINFO stOsVersion;

	stOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&stOsVersion))
		{
		return ( stOsVersion.dwPlatformId == VER_PLATFORM_WIN32_NT );
		}

	return FALSE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  GetWorkingDirectory
 *
 * DESCRIPTION: Determines if we are running under Windows NT
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  0 if user directory not specified
 *
 * Author:  MPT 8-18-99
 */
DWORD GetWorkingDirectory(LPWSTR pszUserDir, DWORD dwSize)
    {
    DWORD   dwRet = MAX_PATH;

    pszUserDir[0] = L'\0';
    if ( regQueryValue(HKEY_CURRENT_USER,
                       L"SOFTWARE\\Hilgraeve Inc\\UltraTerminal PE\\3.0",
                       L"WorkingPath",
                       pszUserDir,
                       &dwRet) != 0 )
		{
        return 0;
		}

	return 1;
	}
