#ifndef ULTRATERMINAL_OPENSSH_CONFIG_H
#define ULTRATERMINAL_OPENSSH_CONFIG_H

#define WITH_OPENSSL 1
#define OPENSSL_HAS_ECC 1
#define OPENSSL_HAS_NISTP256 1
#define OPENSSL_HAS_NISTP384 1
#define OPENSSL_HAS_NISTP521 1
#define HAVE_EVP_DIGESTSIGN 1
#define HAVE_EVP_DIGESTVERIFY 1

#define HAVE_ATTRIBUTE__SENTINEL__ 1
#define HAVE_ATTRIBUTE__NORETURN__ 1
#define HAVE_ATTRIBUTE__UNUSED__ 1
#define HAVE_ATTRIBUTE__BOUNDED__ 0
#define HAVE_ATTRIBUTE__NONNULL__ 1
#define HAVE_ATTRIBUTE__RETURN_TYPE__ 1
#define HAVE_ATTRIBUTE__FORMAT__ 1

#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_UTIME_H 1
#define HAVE_FCNTL_H 1
#define HAVE_STRINGS_H 1
#define HAVE_GETADDRINFO 1
#define HAVE_GETNAMEINFO 1
#define HAVE_GETRRSETBYNAME 1
#define HAVE_GETCWD 1
#define HAVE_REALPATH 1
#define HAVE_INET_NTOA 1
#define HAVE_INET_NTOP 1
#define HAVE_ISBLANK 1
#define HAVE_OPENPTY 1
#define HAVE_PSELECT 1
#define HAVE_MBTOWC 1
#define HAVE_RRESVPORT_AF 1
#define HAVE_WAITPID 1
#define HAVE_STRUCT_POLLFD_FD 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_IN6_ADDR 1
#define HAVE_STRUCT_SOCKADDR_IN6 1
#define HAVE_STRUCT_ADDRINFO 1
#define HAVE_SYS_UN_H 1
#define HAVE_NFDS_T 1
#define HAVE_POLL 1
#define HAVE_PPOLL 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_VA_COPY 1
#define HAVE_STRERROR 1
#define HAVE_STRDUP 1
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_RAISE 1
#define HAVE_SIGNAL 1
#define HAVE_CLOCK 1
#define HAVE_TIME 1
#define HAVE_CTIME 1
#define HAVE_TCGETPGRP 1
#define HAVE_TCSENDBREAK 1
/* #undef HAVE_UTIMES */
/* MinGW's nanosleep inline calls nanosleep64, which we provide in the
 * UltraTerminal Win32 shim to avoid a winpthread runtime dependency. */
#define HAVE_NANOSLEEP 1
/* MinGW advertises clock_gettime through inline/macro names that do not link
 * for us; let OpenSSH fall back to gettimeofday for monotime. */
/* #undef HAVE_CLOCK_GETTIME */
#define HAVE_LONG_LONG_INT 1
#define HAVE_UNSIGNED_LONG_LONG_INT 1
#define HAVE_UINTXX_T 1
#define HAVE_INTXX_T 1
#define HAVE_U_INT 1
#define HAVE_U_CHAR 1
#define HAVE_U_SHORT 1
#define HAVE_U_LONG 1
#define HAVE_SIZE_T 1
#define HAVE_SSIZE_T 1
#define HAVE_MODE_T 1
#define HAVE_PID_T 1
#define HAVE_SA_FAMILY_T 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_STRUCT_TIMEVAL 1

#define HAVE_DECL_SHUT_RD 1
#define HAVE_DECL_O_NONBLOCK 1
#define HAVE_DECL_MAXSYMLINKS 0
#define HAVE_DECL_MEMMEM 0
#define HAVE_DECL_FTRUNCATE 0
#define HAVE_DECL_BZERO 0

/* Autoconf-style feature tests must be undefined when unavailable. */
/* #undef HAVE_STRLCPY */
/* #undef HAVE_STRLCAT */
/* #undef HAVE_REALLOCARRAY */
/* #undef HAVE_RECALLOCARRAY */
/* #undef HAVE_FREEZERO */
/* #undef HAVE_EXPLICIT_BZERO */
/* #undef HAVE_TIMINGSAFE_BCMP */
/* #undef HAVE_STRTONUM */
/* #undef HAVE_B64_NTOP */
/* #undef HAVE_B64_PTON */
/* #undef HAVE_ARC4RANDOM */
/* #undef HAVE_ARC4RANDOM_BUF */
/* #undef HAVE_ARC4RANDOM_UNIFORM */
/* #undef HAVE_GETENTROPY */
/* #undef HAVE_GETLINE */
/* #undef HAVE_STRNDUP */
/* #undef HAVE_STRNLEN */
/* #undef HAVE_STRCASESTR */
/* #undef HAVE_STRSEP */
/* #undef HAVE_SETENV */
/* #undef HAVE_UNSETENV */
/* #undef HAVE_GETPAGESIZE */
#define BROKEN_POLL 0

#define DISABLE_FD_PASSING 1
#define DISABLE_LASTLOG 1
#define DISABLE_LOGIN 1
#define DISABLE_PUTUTLINE 1
#define DISABLE_PUTUTXLINE 1
#define DISABLE_SHADOW 1
#define DISABLE_UTMP 1
#define DISABLE_UTMPX 1
#define DISABLE_WTMP 1
#define DISABLE_WTMPX 1

#define SSH_PROGRAM "UltraTerminal"
#define _PATH_SSH_PROGRAM "UltraTerminal"
#define _PATH_SSH_USER_CONFFILE ".ssh/config"
#define _PATH_SSH_USER_HOSTFILE ".ssh/known_hosts"
#define _PATH_SSH_SYSTEM_HOSTFILE "ssh_known_hosts"
#define _PATH_DEVNULL "NUL"
#define _PATH_TTY "CON"
#define _PATH_BSHELL "cmd.exe"
#define _PATH_NOLOGIN "NUL"

#define SSH_PRIVSEP_USER "nobody"
#define SSH_TUNABLES "NUL"
#define SANDBOX_SKIP_RLIMIT 1

#include <direct.h>
#define mkdir(path, mode) _mkdir(path)

#define SIZEOF_INT 4
#define SIZEOF_LONG_INT 4
#define SIZEOF_LONG_LONG_INT 8

#endif
