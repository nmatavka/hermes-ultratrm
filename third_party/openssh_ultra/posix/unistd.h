#ifndef ULTRATERMINAL_POSIX_UNISTD_H
#define ULTRATERMINAL_POSIX_UNISTD_H

#include <stddef.h>
#include <sys/types.h>

typedef int uid_t;
typedef int gid_t;

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

int close(int fd);
int pipe(int fds[2]);
pid_t fork(void);
pid_t getppid(void);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
unsigned int sleep(unsigned int seconds);
char *realpath(const char *path, char *resolved_path);

#endif
