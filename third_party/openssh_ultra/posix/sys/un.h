#ifndef ULTRATERMINAL_OPENSSH_POSIX_SYS_UN_H
#define ULTRATERMINAL_OPENSSH_POSIX_SYS_UN_H

#include <sys/socket.h>

struct sockaddr_un
    {
    sa_family_t sun_family;
    char sun_path[108];
    };

#endif
