#ifndef ULTRATERMINAL_OPENSSH_POSIX_GLOB_H
#define ULTRATERMINAL_OPENSSH_POSIX_GLOB_H

#include <stddef.h>

typedef struct
    {
    size_t gl_pathc;
    char **gl_pathv;
    size_t gl_offs;
    } glob_t;

#define GLOB_ERR 0x0001
#define GLOB_MARK 0x0002
#define GLOB_NOSORT 0x0004
#define GLOB_NOCHECK 0x0008
#define GLOB_TILDE 0x0010
#define GLOB_ALTDIRFUNC 0x0020

#define GLOB_NOMATCH 1
#define GLOB_ABORTED 2
#define GLOB_NOSPACE 3

int glob(const char *pattern, int flags, int (*errfunc)(const char *, int),
        glob_t *pglob);
void globfree(glob_t *pglob);

#endif
