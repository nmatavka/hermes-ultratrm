#ifndef ULTRATERMINAL_POSIX_POLL_H
#define ULTRATERMINAL_POSIX_POLL_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <time.h>

typedef unsigned int nfds_t;

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout,
    const void *sigmask);

#endif
