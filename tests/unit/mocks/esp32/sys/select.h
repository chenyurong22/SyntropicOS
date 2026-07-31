#ifndef MOCK_SYS_SELECT_H
#define MOCK_SYS_SELECT_H

#include <stdint.h>
#include <sys/socket.h>

struct timeval {
    long tv_sec;
    long tv_usec;
};

typedef struct {
    uint32_t fds_bits[1];
} fd_set;

#define FD_ZERO(s) (void)(s)
#define FD_SET(f, s) \
    (void)(f);       \
    (void)(s)
#define FD_ISSET(f, s) 0

static inline int select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    (void)n;
    (void)r;
    (void)w;
    (void)e;
    (void)t;
    return 0;
}

#endif /* MOCK_SYS_SELECT_H */
