#ifndef MOCK_PICO_STDLIB_H
#define MOCK_PICO_STDLIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICO_ERROR_TIMEOUT (-1)

static inline void stdio_init_all(void)
{
}
static inline void sleep_ms(uint32_t ms)
{
    (void)ms;
}
static inline uint32_t time_us_32(void)
{
    return 0;
}
static inline int getchar_timeout_us(uint32_t us)
{
    (void)us;
    return PICO_ERROR_TIMEOUT;
}

#endif /* MOCK_PICO_STDLIB_H */
