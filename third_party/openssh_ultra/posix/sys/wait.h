#ifndef ULTRATERMINAL_OPENSSH_POSIX_SYS_WAIT_H
#define ULTRATERMINAL_OPENSSH_POSIX_SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG 1
#define WUNTRACED 2

#ifndef WIFEXITED
#define WIFEXITED(status) (((status) & 0x7f) == 0)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(status) (((status) & 0x7f) != 0)
#endif
#ifndef WTERMSIG
#define WTERMSIG(status) ((status) & 0x7f)
#endif

pid_t waitpid(pid_t pid, int *status, int options);

#endif
