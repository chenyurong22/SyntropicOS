#ifndef MOCK_NETDB_H
#define MOCK_NETDB_H

#include <sys/socket.h>

struct hostent {
    char *h_name;
    char **h_aliases;
    int h_addrtype;
    int h_length;
    char **h_addr_list;
};
#define h_addr h_addr_list[0]

struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};

static inline struct hostent *gethostbyname(const char *name)
{
    (void)name;
    static struct hostent h;
    static char *addrs[2] = {NULL, NULL};
    h.h_addr_list = addrs;
    return &h;
}

static inline int getaddrinfo(const char *n, const char *s, const struct addrinfo *h,
                              struct addrinfo **r)
{
    (void)n;
    (void)s;
    (void)h;
    static struct addrinfo ai;
    static struct sockaddr sa;
    ai.ai_addr = &sa;
    *r = &ai;
    return 0;
}

static inline void freeaddrinfo(struct addrinfo *res)
{
    (void)res;
}

#endif /* MOCK_NETDB_H */
