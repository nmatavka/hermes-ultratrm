#ifndef ULTRATERMINAL_OPENSSH_POSIX_FCNTL_H
#define ULTRATERMINAL_OPENSSH_POSIX_FCNTL_H

#include_next <fcntl.h>

#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef F_SETFD
#define F_SETFD 2
#endif
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

int fcntl(int fd, int cmd, ...);

#endif
