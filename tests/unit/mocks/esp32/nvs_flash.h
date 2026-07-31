#ifndef MOCK_NVS_FLASH_H
#define MOCK_NVS_FLASH_H

#include <stdint.h>

#define ESP_ERR_NVS_NO_FREE_PAGES 0x1101
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1102

static inline int nvs_flash_init(void)
{
    return 0;
}
static inline int nvs_flash_erase(void)
{
    return 0;
}

#endif /* MOCK_NVS_FLASH_H */
