#ifndef ULTRATERMINAL_POSIX_SYS_SOCKET_H
#define ULTRATERMINAL_POSIX_SYS_SOCKET_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

typedef int socklen_t;
typedef ADDRESS_FAMILY sa_family_t;

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#endif

#endif
