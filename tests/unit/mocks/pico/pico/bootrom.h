#ifndef MOCK_PICO_BOOTROM_H
#define MOCK_PICO_BOOTROM_H

#include <stdint.h>

static inline void reset_usb_boot(uint32_t gpio, uint32_t flags)
{
    (void)gpio;
    (void)flags;
}

#endif /* MOCK_PICO_BOOTROM_H */
