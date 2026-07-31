#ifndef MOCK_ESP_FLASH_H
#define MOCK_ESP_FLASH_H

#include "esp_wifi.h"

#include <stddef.h>
#include <stdint.h>

static inline esp_err_t esp_flash_erase_region(void *t, uint32_t a, uint32_t s)
{
    (void)t;
    (void)a;
    (void)s;
    return ESP_OK;
}
static inline esp_err_t esp_flash_read(void *t, void *b, uint32_t a, uint32_t len)
{
    (void)t;
    (void)b;
    (void)a;
    (void)len;
    return ESP_OK;
}
static inline esp_err_t esp_flash_write(void *t, const void *b, uint32_t a, uint32_t len)
{
    (void)t;
    (void)b;
    (void)a;
    (void)len;
    return ESP_OK;
}

#endif /* MOCK_ESP_FLASH_H */
