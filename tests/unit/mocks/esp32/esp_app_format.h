#ifndef MOCK_ESP_APP_FORMAT_H
#define MOCK_ESP_APP_FORMAT_H

#include <stdint.h>

typedef struct {
    char magic[4];
} esp_app_desc_t;

static inline const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t d = {"ESP"};
    return &d;
}

#endif /* MOCK_ESP_APP_FORMAT_H */
