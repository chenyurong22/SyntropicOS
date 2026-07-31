#ifndef MOCK_ESP_EVENT_H
#define MOCK_ESP_EVENT_H

#include <stdint.h>

#define ESP_EVENT_ANY_ID (-1)
#define WIFI_EVENT 0
#define IP_EVENT 0
#define IP_EVENT_STA_GOT_IP 1

typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
typedef void (*esp_event_handler_t)(void *event_handler_arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);

static inline int esp_event_handler_instance_register(esp_event_base_t b, int32_t id,
                                                      esp_event_handler_t h, void *arg,
                                                      esp_event_handler_instance_t *i)
{
    (void)b;
    (void)id;
    (void)h;
    (void)arg;
    (void)i;
    return 0;
}

static inline int esp_event_handler_register(esp_event_base_t b, int32_t id, esp_event_handler_t h,
                                             void *arg)
{
    (void)b;
    (void)id;
    (void)h;
    (void)arg;
    return 0;
}

#endif /* MOCK_ESP_EVENT_H */
