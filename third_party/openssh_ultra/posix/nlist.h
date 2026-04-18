#ifndef ULTRATERMINAL_OPENSSH_POSIX_NLIST_H
#define ULTRATERMINAL_OPENSSH_POSIX_NLIST_H

struct nlist
    {
    char *n_name;
    long n_value;
    short n_type;
    short n_desc;
    };

#endif
