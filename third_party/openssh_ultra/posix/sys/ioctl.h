#ifndef ULTRATERMINAL_OPENSSH_POSIX_SYS_IOCTL_H
#define ULTRATERMINAL_OPENSSH_POSIX_SYS_IOCTL_H

#define TIOCGPGRP 0x540F
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#define SIOCGIFFLAGS 0x8913
#define SIOCSIFFLAGS 0x8914

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

int ioctl(int fd, unsigned long request, ...);

#endif
