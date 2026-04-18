#include <winsock2.h>
#include <windows.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <process.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#include <sys/types.h>

#include "posix/pwd.h"
#include "posix/poll.h"
#include "posix/signal.h"
#include "posix/glob.h"
#include "posix/ifaddrs.h"
#include "posix/fcntl.h"
#include "posix/sys/wait.h"
#include "posix/sys/ioctl.h"
#include "posix/termios.h"
#include "posix/sys/mman.h"
#include "posix/sys/uio.h"

#include "includes.h"
#include "ssherr.h"
#include "sshkey.h"
#include <openssl/evp.h>

static int ut_is_leap_year(int year)
    {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

static int ut_month_days(int year, int month)
    {
    static const int days[12] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 1 && ut_is_leap_year(year))
        return 29;
    return days[month];
    }

static int ut_parse_fixed_int(const char **text, int digits, int *value)
    {
    int i;
    int out;

    out = 0;
    for (i = 0; i < digits; ++i)
        {
        if ((*text)[i] < '0' || (*text)[i] > '9')
            return 0;
        out = out * 10 + ((*text)[i] - '0');
        }
    *text += digits;
    *value = out;
    return 1;
    }

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
    {
    nfds_t i;

    if (fds == 0)
        {
        Sleep(timeout < 0 ? 0 : (DWORD)timeout);
        return 0;
        }

    for (i = 0; i < nfds; ++i)
        fds[i].revents = 0;
    Sleep(timeout < 0 ? 1 : (DWORD)timeout);
    return 0;
    }

int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout,
        const void *sigmask)
    {
    int ms;
    (void)sigmask;

    if (timeout == 0)
        ms = -1;
    else
        ms = (int)(timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000);
    return poll(fds, nfds, ms);
    }

char *strptime(const char *buf, const char *fmt, struct tm *tm)
    {
    const char *b;
    const char *f;
    int value;

    if (buf == 0 || fmt == 0 || tm == 0)
        return 0;
    b = buf;
    f = fmt;
    while (*f != '\0')
        {
        if (*f != '%')
            {
            if (*b != *f)
                return 0;
            ++b;
            ++f;
            continue;
            }
        ++f;
        switch (*f)
            {
            case 'Y':
                if (!ut_parse_fixed_int(&b, 4, &value))
                    return 0;
                tm->tm_year = value - 1900;
                break;
            case 'm':
                if (!ut_parse_fixed_int(&b, 2, &value) ||
                        value < 1 || value > 12)
                    return 0;
                tm->tm_mon = value - 1;
                break;
            case 'd':
                if (!ut_parse_fixed_int(&b, 2, &value) ||
                        value < 1 || value > 31)
                    return 0;
                tm->tm_mday = value;
                break;
            case 'H':
                if (!ut_parse_fixed_int(&b, 2, &value) ||
                        value < 0 || value > 23)
                    return 0;
                tm->tm_hour = value;
                break;
            case 'M':
                if (!ut_parse_fixed_int(&b, 2, &value) ||
                        value < 0 || value > 59)
                    return 0;
                tm->tm_min = value;
                break;
            case 'S':
                if (!ut_parse_fixed_int(&b, 2, &value) ||
                        value < 0 || value > 60)
                    return 0;
                tm->tm_sec = value;
                break;
            default:
                return 0;
            }
        ++f;
        }
    return (char *)b;
    }

time_t timegm(struct tm *tm)
    {
#ifdef _WIN64
    return (time_t)_mkgmtime64(tm);
#else
    return (time_t)_mkgmtime(tm);
#endif
    }

char *strsignal(int sig)
    {
    static char buffer[32];

    _snprintf(buffer, sizeof(buffer), "signal %d", sig);
    buffer[sizeof(buffer) - 1] = '\0';
    return buffer;
    }

pid_t fork(void)
    {
    errno = ENOSYS;
    return -1;
    }

pid_t waitpid(pid_t pid, int *status, int options)
    {
    (void)pid;
    (void)options;
    if (status != 0)
        *status = 1;
    errno = ECHILD;
    return -1;
    }

void closefrom(int lowfd)
    {
    (void)lowfd;
    }

int initgroups(const char *user, gid_t group)
    {
    (void)user;
    (void)group;
    return 0;
    }

int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
    {
    (void)rgid;
    (void)egid;
    (void)sgid;
    return 0;
    }

int setresuid(uid_t ruid, uid_t euid, uid_t suid)
    {
    (void)ruid;
    (void)euid;
    (void)suid;
    return 0;
    }

int sigemptyset(sigset_t *set)
    {
    if (set == 0)
        {
        errno = EINVAL;
        return -1;
        }
    *set = 0;
    return 0;
    }

int sigfillset(sigset_t *set)
    {
    if (set == 0)
        {
        errno = EINVAL;
        return -1;
        }
    *set = (sigset_t)~0;
    return 0;
    }

int sigaddset(sigset_t *set, int signo)
    {
    if (set == 0 || signo < 0)
        {
        errno = EINVAL;
        return -1;
        }
    *set |= ((sigset_t)1 << (signo & 31));
    return 0;
    }

int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact)
    {
    if (oldact != 0)
        {
        oldact->sa_handler = SIG_DFL;
        oldact->sa_mask = 0;
        oldact->sa_flags = 0;
        }
    if (act != 0 && act->sa_handler != 0)
        signal(signo, act->sa_handler);
    return 0;
    }

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
    {
    (void)how;
    (void)set;
    if (oldset != 0)
        *oldset = 0;
    return 0;
    }

int kill(pid_t pid, int signo)
    {
    if (pid == getpid())
        return raise(signo);
    errno = ENOSYS;
    return -1;
    }

int fcntl(int fd, int cmd, ...)
    {
    (void)fd;
    switch (cmd)
        {
        case F_GETFL:
            return 0;
        case F_SETFL:
            return 0;
        case F_SETFD:
            return 0;
        default:
            errno = ENOSYS;
            return -1;
        }
    }

int pipe(int fds[2])
    {
    return _pipe(fds, 4096, _O_BINARY);
    }

int getpagesize(void)
    {
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    return si.dwPageSize != 0 ? (int)si.dwPageSize : 4096;
    }

int nanosleep64(const struct _timespec64 *request, struct _timespec64 *remain)
    {
    ULONGLONG ms;

    if (request == 0)
        {
        errno = EINVAL;
        return -1;
        }
    if (request->tv_sec < 0 || request->tv_nsec < 0 ||
            request->tv_nsec >= 1000000000L)
        {
        errno = EINVAL;
        return -1;
        }
    ms = (ULONGLONG)request->tv_sec * 1000ULL +
            (ULONGLONG)(request->tv_nsec + 999999L) / 1000000ULL;
    Sleep((DWORD)(ms > 0xffffffffULL ? 0xffffffffUL : ms));
    if (remain != 0)
        {
        remain->tv_sec = 0;
        remain->tv_nsec = 0;
        }
    return 0;
    }

int chown(const char *path, uid_t owner, gid_t group)
    {
    (void)path;
    (void)owner;
    (void)group;
    return 0;
    }

int link(const char *oldpath, const char *newpath)
    {
    if (oldpath == 0 || newpath == 0)
        {
        errno = EINVAL;
        return -1;
        }
    if (CreateHardLinkA(newpath, oldpath, 0))
        return 0;
    errno = EIO;
    return -1;
    }

int lstat(const char *path, struct stat *st)
    {
    return stat(path, st);
    }

int EVP_CIPHER_CTX_get_iv(const EVP_CIPHER_CTX *ctx, unsigned char *iv,
        size_t len)
    {
    return EVP_CIPHER_CTX_get_updated_iv((EVP_CIPHER_CTX *)ctx, iv, len);
    }

int EVP_CIPHER_CTX_set_iv(EVP_CIPHER_CTX *ctx, const unsigned char *iv,
        size_t len)
    {
    return EVP_CipherInit_ex(ctx, 0, 0, 0, iv, -1);
    }

uid_t getuid(void)
    {
    return 1000;
    }

uid_t geteuid(void)
    {
    return 1000;
    }

gid_t getgid(void)
    {
    return 1000;
    }

gid_t getegid(void)
    {
    return 1000;
    }

pid_t getppid(void)
    {
    return 1;
    }

char *realpath(const char *path, char *resolved_path)
    {
    char *out;

    if (path == 0)
        {
        errno = EINVAL;
        return 0;
        }
    out = resolved_path != 0 ? resolved_path : malloc(MAX_PATH);
    if (out == 0)
        {
        errno = ENOMEM;
        return 0;
        }
    if (_fullpath(out, path, MAX_PATH) == 0)
        {
        if (resolved_path == 0)
            free(out);
        return 0;
        }
    return out;
    }

struct passwd *getpwuid(uid_t uid)
    {
    static struct passwd pw;
    static char user[128];
    static char home[MAX_PATH];
    DWORD cchUser;
    DWORD cchHome;

    (void)uid;
    cchUser = sizeof(user);
    if (!GetUserNameA(user, &cchUser))
        strcpy(user, "user");

    cchHome = GetEnvironmentVariableA("USERPROFILE", home, sizeof(home));
    if (cchHome == 0 || cchHome >= sizeof(home))
        strcpy(home, ".");

    pw.pw_name = user;
    pw.pw_passwd = "";
    pw.pw_uid = 1000;
    pw.pw_gid = 1000;
    pw.pw_gecos = user;
    pw.pw_dir = home;
    pw.pw_shell = "cmd.exe";
    return &pw;
    }

struct passwd *getpwnam(const char *name)
    {
    struct passwd *pw;

    pw = getpwuid(1000);
    if (name != 0 && name[0] != '\0')
        pw->pw_name = (char *)name;
    return pw;
    }

int tcgetattr(int fd, struct termios *t)
    {
    (void)fd;
    if (t == 0)
        {
        errno = EINVAL;
        return -1;
        }
    memset(t, 0, sizeof(*t));
    t->c_cflag = CS8;
    return 0;
    }

int tcsetattr(int fd, int optional_actions, const struct termios *t)
    {
    (void)fd;
    (void)optional_actions;
    (void)t;
    return 0;
    }

speed_t cfgetispeed(const struct termios *t)
    {
    (void)t;
    return 38400;
    }

speed_t cfgetospeed(const struct termios *t)
    {
    (void)t;
    return 38400;
    }

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
    int i;
    ssize_t total;
    int r;

    total = 0;
    for (i = 0; i < iovcnt; ++i)
        {
        r = _read(fd, iov[i].iov_base, (unsigned int)iov[i].iov_len);
        if (r < 0)
            return total > 0 ? total : -1;
        total += r;
        if ((size_t)r != iov[i].iov_len)
            break;
        }
    return total;
    }

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
    int i;
    ssize_t total;
    int r;

    total = 0;
    for (i = 0; i < iovcnt; ++i)
        {
        r = _write(fd, iov[i].iov_base, (unsigned int)iov[i].iov_len);
        if (r < 0)
            return total > 0 ? total : -1;
        total += r;
        if ((size_t)r != iov[i].iov_len)
            break;
        }
    return total;
    }

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
        long long offset)
    {
    (void)addr;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    if (length == 0)
        return MAP_FAILED;
    return malloc(length);
    }

int munmap(void *addr, size_t length)
    {
    (void)length;
    free(addr);
    return 0;
    }

int ioctl(int fd, unsigned long request, ...)
    {
    (void)fd;
    (void)request;
    errno = ENOSYS;
    return -1;
    }

void openlog(const char *ident, int option, int facility)
    {
    (void)ident;
    (void)option;
    (void)facility;
    }

void closelog(void)
    {
    }

void syslog(int priority, const char *format, ...)
    {
    char buffer[1024];
    va_list ap;

    (void)priority;
    va_start(ap, format);
    _vsnprintf(buffer, sizeof(buffer) - 3, format, ap);
    va_end(ap);
    buffer[sizeof(buffer) - 3] = '\0';
    strcat(buffer, "\r\n");
    OutputDebugStringA(buffer);
    }

int getifaddrs(struct ifaddrs **ifap)
    {
    if (ifap != 0)
        *ifap = 0;
    errno = ENOSYS;
    return -1;
    }

void freeifaddrs(struct ifaddrs *ifa)
    {
    (void)ifa;
    }

int glob(const char *pattern, int flags, int (*errfunc)(const char *, int),
        glob_t *pglob)
    {
    (void)pattern;
    (void)flags;
    (void)errfunc;
    if (pglob != 0)
        {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = 0;
        pglob->gl_offs = 0;
        }
    return GLOB_NOMATCH;
    }

void globfree(glob_t *pglob)
    {
    if (pglob != 0)
        {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = 0;
        pglob->gl_offs = 0;
        }
    }

int _ssh__compat_glob(const char *pattern, int flags,
        int (*errfunc)(const char *, int), glob_t *pglob)
    {
    return glob(pattern, flags, errfunc, pglob);
    }

void _ssh__compat_globfree(glob_t *pglob)
    {
    globfree(pglob);
    }

int scan_scaled(char *scaled, long long *result)
    {
    char *end;
    long long value;

    if (scaled == 0 || result == 0)
        {
        errno = EINVAL;
        return -1;
        }
    value = _strtoi64(scaled, &end, 10);
    if (end == scaled)
        return -1;
    switch (*end)
        {
        case 'k':
        case 'K':
            value *= 1024LL;
            ++end;
            break;
        case 'm':
        case 'M':
            value *= 1024LL * 1024LL;
            ++end;
            break;
        case 'g':
        case 'G':
            value *= 1024LL * 1024LL * 1024LL;
            ++end;
            break;
        default:
            break;
        }
    if (*end != '\0')
        return -1;
    *result = value;
    return 0;
    }

int sshsk_sign(const char *provider_path, struct sshkey *key,
        u_char **sigp, size_t *lenp, const u_char *data, size_t datalen,
        u_int compat, const char *pin)
    {
    (void)provider_path;
    (void)key;
    (void)data;
    (void)datalen;
    (void)compat;
    (void)pin;
    if (sigp != 0)
        *sigp = 0;
    if (lenp != 0)
        *lenp = 0;
    return SSH_ERR_FEATURE_UNSUPPORTED;
    }
