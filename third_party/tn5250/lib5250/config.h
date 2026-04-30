/* UltraTerminal build configuration for the embedded IBM 5250 runtime. */
#ifndef ULTRATERMINAL_IBM5250_CONFIG_H
#define ULTRATERMINAL_IBM5250_CONFIG_H

#ifndef VERSION
#define VERSION "0.19.0-ultraterminal"
#endif
#define PACKAGE "tn5250"
#define PACKAGE_BUGREPORT "https://github.com/tn5250/tn5250/issues"
#define SYSCONFDIR "."

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#define SOCKET_TYPE SOCKET
#else
#define SOCKET_TYPE int
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_FCNTL_H 1
#endif

#if defined(ULTRATERMINAL_IBM5250_HAVE_OPENSSL)
#define HAVE_LIBSSL 1
#define HAVE_LIBCRYPTO 1
#endif

#endif
