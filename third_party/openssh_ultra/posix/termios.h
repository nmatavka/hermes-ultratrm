#ifndef ULTRATERMINAL_POSIX_TERMIOS_H
#define ULTRATERMINAL_POSIX_TERMIOS_H

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

#define NCCS 32

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
};

#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VEOL 5
#define VEOL2 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VREPRINT 10
#define VWERASE 11
#define VLNEXT 12
#define VDISCARD 13

#define IGNBRK 0x0001
#define BRKINT 0x0002
#define IGNPAR 0x0004
#define PARMRK 0x0008
#define INPCK  0x0010
#define ISTRIP 0x0020
#define INLCR  0x0040
#define IGNCR  0x0080
#define ICRNL  0x0100
#define IXON   0x0200
#define IXANY  0x0400
#define IXOFF  0x0800

#define OPOST  0x0001
#define ONLCR  0x0002
#define OCRNL  0x0004
#define ONOCR  0x0008
#define ONLRET 0x0010

#define CS7    0x0020
#define CS8    0x0040
#define PARENB 0x0080
#define PARODD 0x0100

#define ECHO   0x0001
#define ECHONL 0x0002
#define ICANON 0x0004
#define ISIG   0x0008
#define IEXTEN 0x0010
#define NOFLSH 0x0020
#define TOSTOP 0x0040

#define TCSANOW 0
#define TCSAFLUSH 2

int tcgetattr(int fd, struct termios *t);
int tcsetattr(int fd, int optional_actions, const struct termios *t);
speed_t cfgetispeed(const struct termios *t);
speed_t cfgetospeed(const struct termios *t);

#endif
