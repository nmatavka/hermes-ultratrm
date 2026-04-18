#ifndef ULTRATERMINAL_OPENSSH_POSIX_NET_IF_H
#define ULTRATERMINAL_OPENSSH_POSIX_NET_IF_H

#define IFF_UP 0x1

struct ifreq
    {
    char ifr_name[16];
    short ifr_flags;
    };

#endif
