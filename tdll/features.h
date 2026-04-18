/*      File: D:\WACKER\tdll\features.h (Created: 24-Aug-1994)
 *
 *      Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 15 $
 *      $Date: 3/16/01 4:28p $
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *                           R E A D   M E
 *
 * This file is a replacement for the INC.H file that was used in HAWIN
 * and HA/5.  It controls optional features that may or may not be built
 * into this product.  This file CANNOT have anything except defines in
 * it.  It is for control and configuration only.  Violate this rule at
 * your peril.  (Please ?)
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *                           R E A D   M E
 *
 * This file has been organized into sections based on language.  To find
 * which features are enabled search for the language you are building.
 *
 * The following section contains descriptions of the settings currently
 * available for each language.
 *
 * The end of the file contains a series of test to verify that required
 * settings have been set.
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/


#if !defined(FEATURES_H_INCLUDED)
#define FEATURES_H_INCLUDED

#if defined(EXTENDED_FEATURES)
#define INCL_ZMODEM_CRASH_RECOVERY
#define INCL_REDIAL_ON_BUSY
#define INCL_USE_TERMINAL_FONT
#define INCL_SPINNING_GLOBE
#define INCL_PRIVATE_EDITION_BANNER
#define USE_PRIVATE_EDITION_3_BANNER
#define INCL_WINSOCK
#define INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
#define INCL_CALL_ANSWERING
#define INCL_DEFAULT_TELNET_APP
#define INCL_VT100COLORS
#define INCL_EXIT_ON_DISCONNECT
#define INCL_VT220                              // Added 20-Jan-98. rde
#if defined(INCL_VT220)                         // The 320 requires the 220 be defined.
#define INCL_VT320                              // Added 24-Jan-98. rde
#endif
// This next define is for host controlled printing - raw versus windows 
// It should be enabled for commercial builds. MPT 11-18-99
#define INCL_PRINT_PASSTHROUGH
// A customer specific version. Added 16 Feb 98. rde
//#define INCL_ULTC_VERSION                        

//Private Edition 4 features
#define INCL_TERMINAL_SIZE_AND_COLORS
#define INCL_KEY_MACROS
#define INCL_TERMINAL_CLEAR
//#define INCL_USE_HTML_HELP //removed due to a requirement to redistribute a 404k support program
#define INCL_NAG_SCREEN

#endif //EXTENDED_FEATURES

#if defined(NT_EDITION)
#undef  INCL_NAG_SCREEN	// There is no nag screen in the Microsoft version.
#define INCL_ZMODEM_CRASH_RECOVERY
#define INCL_REDIAL_ON_BUSY
#define INCL_USE_TERMINAL_FONT
#undef INCL_SPINNING_GLOBE
#undef INCL_PRIVATE_EDITION_BANNER // There is no banner screen in the Microsoft version.
#undef USE_PRIVATE_EDITION_3_BANNER  // There is no banner screen in the Microsoft version.
#define INCL_WINSOCK
#define INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
#define INCL_CALL_ANSWERING
#define INCL_DEFAULT_TELNET_APP
//mpt:08-22-97 added HTML help for Microsoft's version
#if !defined(ULTRATERMINAL_DISABLE_HTML_HELP)
 #define INCL_USE_HTML_HELP
#endif
//mpt:04-29-98 added new Printing Common Dialogs for Microsoft
#if(WINVER >= 0x0500)
#define INCL_USE_NEWPRINTDLG
#endif // WINVER >= 0x0500

//mpt:09-24-99 added new browse dialog for Microsoft
#define INCL_USE_NEWFOLDERDLG
#define INCL_64BIT
#endif // NT_EDITION


/*
 * Minitel and Prestel terminals are now included standard
 */
#define INCL_MINITEL
#define INCL_VIEWDATA
#if defined(ULTRATERMINAL_ENABLE_VIDEOTEX) && !defined(INCL_VIDEOTEX)
#define INCL_VIDEOTEX
#endif

/*
 * This feature provides code to
 * support an optional character translation DLL.  This DLL is called to
 * translate the data stream on both input and output.  It does not
 * translate the underlying character values (at the present time), only
 * the encoding method.  The initial version of this will only translate
 * between JIS and Shift-JIS.  For commercial release, additional operations
 * such as JIS escape recovery, UNICODE, and EUC encoding can be added just
 * by changing the DLL.  In fact, the new DLL can be offered as an upgrade
 * to the lower version of the product.
 *
 * #define CHARACTER_TRANSLATION
 */

/*
 * UltraTerminal is Unicode-only.  The old app character-width build modes
 * are intentionally gone; host encodings are handled by protocol adapters.
 */
#define CHAR_WIDE
#undef INCL_VT100J
#undef INCL_ANSIW
#define CHARACTER_TRANSLATION
#endif
