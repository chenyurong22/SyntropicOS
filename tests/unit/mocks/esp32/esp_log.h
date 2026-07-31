#ifndef MOCK_ESP_LOG_H
#define MOCK_ESP_LOG_H

#include <stdint.h>

#define ESP_LOGI(tag, fmt, ...) (void)0
#define ESP_LOGE(tag, fmt, ...) (void)0
#define ESP_LOGW(tag, fmt, ...) (void)0

static inline int64_t esp_timer_get_time(void)
{
    return 0;
}
static inline void esp_rom_delay_us(uint32_t us)
{
    (void)us;
}

#endif /* MOCK_ESP_LOG_H */
