#ifndef MOCK_PICO_MULTICORE_H
#define MOCK_PICO_MULTICORE_H

#include <stdint.h>

static inline void multicore_launch_core1(void (*entry)(void))
{
    (void)entry;
}
static inline void multicore_fifo_push_blocking(uint32_t data)
{
    (void)data;
}
static inline uint32_t multicore_fifo_pop_blocking(void)
{
    return 0;
}

#endif /* MOCK_PICO_MULTICORE_H */
