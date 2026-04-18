#ifndef ULTRATERMINAL_OPENSSH_POSIX_SIGNAL_H
#define ULTRATERMINAL_OPENSSH_POSIX_SIGNAL_H

#include_next <signal.h>
#include <sys/types.h>

#ifndef _POSIX
typedef _sigset_t sigset_t;
#endif

#ifndef _NSIG
#define _NSIG 64
#endif

#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGBUS
#define SIGBUS 10
#endif
#ifndef SIGSYS
#define SIGSYS 12
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGALRM
#define SIGALRM 14
#endif
#ifndef SIGCHLD
#define SIGCHLD 20
#endif
#ifndef SIGTSTP
#define SIGTSTP 18
#endif
#ifndef SIGTTIN
#define SIGTTIN 19
#endif
#ifndef SIGTTOU
#define SIGTTOU 20
#endif
#ifndef SIGWINCH
#define SIGWINCH 28
#endif
#ifndef SIGTRAP
#define SIGTRAP 5
#endif

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

typedef void (*ut_signal_handler_t)(int);

#ifndef SIG_DFL
#define SIG_DFL ((ut_signal_handler_t)0)
#endif
#ifndef SIG_IGN
#define SIG_IGN ((ut_signal_handler_t)1)
#endif
#ifndef SIG_ERR
#define SIG_ERR ((ut_signal_handler_t)-1)
#endif

ut_signal_handler_t signal(int signo, ut_signal_handler_t handler);
int raise(int signo);

struct sigaction
    {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    };

int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigaction(int signo, const struct sigaction *act,
        struct sigaction *oldact);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int kill(pid_t pid, int signo);

#endif
