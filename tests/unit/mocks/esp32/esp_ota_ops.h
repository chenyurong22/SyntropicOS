#ifndef MOCK_ESP_OTA_OPS_H
#define MOCK_ESP_OTA_OPS_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t esp_ota_handle_t;
typedef struct {
    uint32_t type;
    const char *label;
    uint32_t address;
    uint32_t size;
} esp_partition_t;

#define ESP_OK 0
#define OTA_WITH_SEQUENTIAL_WRITES 0

static inline const esp_partition_t *esp_ota_get_running_partition(void)
{
    static const esp_partition_t p = {0, "ota_0", 0x10000, 0x100000};
    return &p;
}

static inline const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start)
{
    (void)start;
    static const esp_partition_t p = {0, "ota_1", 0x110000, 0x100000};
    return &p;
}

static inline int esp_ota_begin(const esp_partition_t *p, size_t sz, esp_ota_handle_t *h)
{
    (void)p;
    (void)sz;
    (void)h;
    return ESP_OK;
}

static inline int esp_ota_write(esp_ota_handle_t h, const void *data, size_t sz)
{
    (void)h;
    (void)data;
    (void)sz;
    return ESP_OK;
}

static inline int esp_ota_end(esp_ota_handle_t h)
{
    (void)h;
    return ESP_OK;
}

static inline int esp_ota_abort(esp_ota_handle_t h)
{
    (void)h;
    return ESP_OK;
}

static inline int esp_ota_set_boot_partition(const esp_partition_t *p)
{
    (void)p;
    return ESP_OK;
}

#endif /* MOCK_ESP_OTA_OPS_H */
