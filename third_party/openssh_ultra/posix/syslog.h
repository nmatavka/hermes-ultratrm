#ifndef ULTRATERMINAL_OPENSSH_POSIX_SYSLOG_H
#define ULTRATERMINAL_OPENSSH_POSIX_SYSLOG_H

#define LOG_PID 0x01
#define LOG_CONS 0x02

#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_INFO 6
#define LOG_DEBUG 7

#define LOG_AUTH 32
#define LOG_DAEMON 24
#define LOG_USER 8
#define LOG_AUTHPRIV 80
#define LOG_LOCAL0 128
#define LOG_LOCAL1 136
#define LOG_LOCAL2 144
#define LOG_LOCAL3 152
#define LOG_LOCAL4 160
#define LOG_LOCAL5 168
#define LOG_LOCAL6 176
#define LOG_LOCAL7 184

void openlog(const char *ident, int option, int facility);
void closelog(void);
void syslog(int priority, const char *format, ...);

#endif
