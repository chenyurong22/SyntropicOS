#ifndef MOCK_ARPA_INET_H
#define MOCK_ARPA_INET_H

#include <sys/socket.h>

static inline uint16_t htons(uint16_t n)
{
    return (uint16_t)((n >> 8) | (n << 8));
}
static inline uint32_t htonl(uint32_t n)
{
    return ((n >> 24) | ((n >> 8) & 0xFF00) | ((n << 8) & 0xFF0000) | (n << 24));
}
static inline uint16_t ntohs(uint16_t n)
{
    return htons(n);
}
static inline uint32_t ntohl(uint32_t n)
{
    return htonl(n);
}
static inline char *inet_ntoa(struct in_addr in)
{
    (void)in;
    return "127.0.0.1";
}
static inline uint32_t inet_addr(const char *cp)
{
    (void)cp;
    return 0;
}

#endif /* MOCK_ARPA_INET_H */
