#ifndef ULTRATERMINAL_OPENSSH_POSIX_SYS_STAT_H
#define ULTRATERMINAL_OPENSSH_POSIX_SYS_STAT_H

#include_next <sys/stat.h>

#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif
#ifndef S_ISUID
#define S_ISUID 04000
#endif
#ifndef S_ISGID
#define S_ISGID 02000
#endif
#ifndef S_ISVTX
#define S_ISVTX 01000
#endif

#endif
